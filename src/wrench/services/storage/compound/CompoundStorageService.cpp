#include <cmath>

#include <wrench/services/storage/compound/CompoundStorageService.h>

#include "wrench/services/storage/StorageServiceMessage.h"
#include "wrench/services/storage/compound/CompoundStorageServiceMessage.h"
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/storage/StorageServiceMessagePayload.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>

WRENCH_LOG_CATEGORY(wrench_core_compound_storage_system,
                    "Log category for Compound Storage Service");

namespace wrench {

    /** @brief The null allocator */
    StorageSelectionStrategyCallback NullAllocator = [](const std::shared_ptr<DataFile> &file,
                                                        const std::map<std::string, std::vector<std::shared_ptr<StorageService>>> &resources,
                                                        const std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> &mapping,
                                                        const std::vector<std::shared_ptr<FileLocation>> &previous_allocations,
                                                        unsigned int stripe_count) {
        return std::vector<std::shared_ptr<FileLocation>>();
    };

    /**
     *  @brief Constructor for the case where no request message (for I/O operations) should ever reach
     *         the CompoundStorageService. This use case suppose that any action making use of a FileLocation
     *         referencing this CompoundStorageService will be intercepted before its execution (in a scheduler
     *         for instance) and updated with one of the StorageServices known to this CompoundStorageService.
     *
     *  @param hostname: the name of the host on which this service will run
     *  @param storage_services: subordinate storage services
     *  @param property_list: the configurable properties
     *  @param messagepayload_list: the configurable message payloads
     */

    CompoundStorageService::CompoundStorageService(const std::string &hostname,
                                                   std::set<std::shared_ptr<StorageService>> storage_services,
                                                   WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                   WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE messagepayload_list) : CompoundStorageService(hostname, storage_services, NullAllocator,
                                                                                                                                       property_list, messagepayload_list){};

    /**
     *  @brief Constructor
     *  @param hostname: the name of the host on which this service will run
     *  @param storage_services: subordinate storage services
     *  @param allocate: the storage allocation strategy
     *  @param property_list: the configurable properties
     *  @param messagepayload_list: the configurable message payloads
     */
    CompoundStorageService::CompoundStorageService(
            const std::string &hostname,
            std::set<std::shared_ptr<StorageService>> storage_services,
            StorageSelectionStrategyCallback &allocate,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            const WRENCH_MESSAGE_PAYLOAD_COLLECTION_TYPE& messagepayload_list) : StorageService(hostname, "compound_storage_" + std::to_string(getNewUniqueNumber())), allocate(allocate) {
        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        if (storage_services.empty()) {
            throw std::invalid_argument("Got an empty list of StorageServices for CompoundStorageService."
                                        "Must specify at least one valid StorageService");
        }

        if (std::any_of(storage_services.begin(), storage_services.end(), [](const auto &elem) { return elem == nullptr; })) {
            throw std::invalid_argument("One of the StorageServices provided is not initialized");
        }

        /* For now, we do not allow storage services that are simple with more than one mount point */
        if (std::any_of(storage_services.begin(), storage_services.end(), [](const auto &elem) {
          auto sss = std::dynamic_pointer_cast<SimpleStorageService>(elem);
          return sss->hasMultipleMountPoints(); })) {
            throw std::invalid_argument("One of the SimpleStorageServices provided has more than one mount point. "
                                        "In the current state of the implementation this is currently not allowed");
        }

        for (const auto &storage_service: storage_services) {
            if (this->storage_services.find(storage_service->getHostname()) != this->storage_services.end()) {
                this->storage_services[storage_service->getHostname()].push_back(storage_service);
            } else {
                this->storage_services[storage_service->getHostname()] = std::vector<std::shared_ptr<wrench::StorageService>>{storage_service};
            }
            this->total_nb_storage_services++;
        }

        if (property_list.find(wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE) == property_list.end()) {
            // If MAX_ALLOCATION_CHUNK_SIZE was not provided, update it now that we have validated the SSS list
            // (Set as smallest disk capacity in bytes for instance ?)
            // TODO
            // this->setProperty(CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "2000000B");
        }

        this->max_chunk_size = this->getPropertyValueAsSizeInByte(CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE);
        this->internal_stripping = this->getPropertyValueAsBoolean(CompoundStorageServiceProperty::INTERNAL_STRIPING);
        // this->traceInternalStorageUse(IOAction::None);
    }

    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int CompoundStorageService::main() {
        // TODO: Use another color?
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);
        std::string message = "Compound Storage Service " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());

        WRENCH_INFO("CSS - Registered underlying storage services:");
        for (const auto &storage_server: this->storage_services) {
            for (const auto &service: storage_server.second) {
                message = " - " + service->process_name + " on " + service->getHostname() +
                        " (" + std::to_string(service->getTotalFreeSpaceZeroTime()) + " bytes)";
                WRENCH_INFO("%s", message.c_str());
            }
        }

        /** Main loop **/
        bool comm_ptr_has_been_posted = false;
        simgrid::s4u::CommPtr comm_ptr;
        std::unique_ptr<SimulationMessage> simulation_message;
        while (true) {
            S4U_Simulation::computeZeroFlop();

            simulation_message = this->commport->getMessage<SimulationMessage>();

            if (not processNextMessage(simulation_message.get()))
                break;
        }

        WRENCH_INFO("Compound Storage Service %s on host %s cleanly terminating!",
                    this->getName().c_str(),
                    S4U_Simulation::getHostName().c_str());

        return 0;
    }

    /**
     * @brief Process a received control message
     *
     * @param message: the simulation message to process
     *
     * @return false if the daemon should terminate
     */
    bool CompoundStorageService::processNextMessage(SimulationMessage *message) {
        WRENCH_INFO("CSS::Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message)) {
            return processStopDaemonRequest(msg->ack_commport);
        } else if (auto msg = dynamic_cast<CompoundStorageAllocationRequestMessage *>(message)) {
            WRENCH_INFO("Calling processStorageSelectionMessage for file :  %s", msg->file->getID().c_str());
            return processStorageSelectionMessage(msg);
        } else if (auto msg = dynamic_cast<CompoundStorageLookupRequestMessage *>(message)) {
            WRENCH_INFO("Calling processStorageLookupMessage for file :  %s", msg->file->getID().c_str());
            return processStorageLookupMessage(msg);
        } else {
            throw std::runtime_error(
                    "CSS::processNextMessage(): Unexpected [" + message->getName() + "] message." +
                    "This is only an abstraction layer and it can't be used as an actual storage service");
        }
    }

    bool CompoundStorageService::processStorageSelectionMessage(const CompoundStorageAllocationRequestMessage *msg) {
        auto file = msg->file;
        WRENCH_INFO("CSS::processStorageSelectionMessage(): For file %s", file->getID().c_str());

        if (this->file_location_mapping.find(file) != this->file_location_mapping.end()) {
            WRENCH_INFO("CSS::processStorageSelectionMessage: File %s already known by CSS", file->getID().c_str());

            msg->answer_commport->dputMessage(
                    new CompoundStorageAllocationAnswerMessage(
                            this->file_location_mapping[file],
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));
            return true;
        }

        std::vector<std::shared_ptr<DataFile>> parts = {};
        auto file_size = file->getSize();
        auto file_name = file->getID();

        // Stripping case
        if (file_size > this->max_chunk_size && this->internal_stripping) {
            WRENCH_INFO("CSS::processStorageSelectionMessage(): Stripping file before sending to allocator");
            sg_size_t remaining = file_size;
            auto part_id = 0;
            while (remaining > this->max_chunk_size) {
                std::string part_file_name = file_name + "_stripe_" + std::to_string(part_id);
                std::shared_ptr<DataFile> part_file = wrench::Simulation::getFileByIDOrNull(part_file_name);
                if (not part_file) {
                    part_file = Simulation::addFile(part_file_name, this->max_chunk_size);
                }
                parts.push_back(part_file);
//                parts.push_back(
//                        this->simulation_->addFile(file_name + "_stripe_" + std::to_string(part_id), this->max_chunk_size));
                part_id++;
                remaining -= this->max_chunk_size;
            }
            std::string part_file_name = file_name + "_stripe_" + std::to_string(part_id);
            std::shared_ptr<DataFile> part_file = wrench::Simulation::getFileByIDOrNull(part_file_name);
            if (not part_file) {
                part_file = Simulation::addFile(part_file_name, remaining);
            }
            parts.push_back(part_file);
        } else {
            parts.push_back(file);
        }

        // Resolve allocations for all parts (possibly only one part)
        std::vector<std::shared_ptr<FileLocation>> designated_locations = {};
        for (const auto &part: parts) {
            WRENCH_DEBUG("CSS::processStorageSelectionMessage(): File %s NOT already known by CSS", part->getID().c_str());
            auto locations = this->allocate(part, this->storage_services, this->file_location_mapping, designated_locations, msg->stripe_count);
            if (!locations.empty()) {
                for (auto new_loc: locations) {
                    bool success = new_loc->getStorageService()->reserveSpace(new_loc);
                    designated_locations.push_back(new_loc);
                }
            } else {
                WRENCH_WARN("CSS::processStorageSelectionMessage(): File %s (or parts) could not be placed on any ss", file->getID().c_str());
                //UNRESERVE ALL PREVIOUSLY RESERVED LOCATIONS
                for (auto &d : designated_locations) {
                    d->getStorageService()->unreserveSpace(d);
                }
                designated_locations = {};
                break;
            }
        }
        if (!designated_locations.empty()) {
            this->file_location_mapping[file] = designated_locations;
            this->partial_io_stripe_index[file] = 0;
            WRENCH_INFO("CSS::processStorageSelectionMessage(): Local mapping updated for file %s", file->getID().c_str());
        }
        msg->answer_commport->dputMessage(
                new CompoundStorageAllocationAnswerMessage(
                        designated_locations,
                        this->getMessagePayloadValue(
                                CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        WRENCH_INFO("CSS::processStorageSelectionMessage(): Answer sent, file striped in %lu stripes (roughly %llu bytes each)", designated_locations.size(), designated_locations[0]->getFile()->getSize());

        return true;
    }

    bool CompoundStorageService::processStorageLookupMessage(const CompoundStorageLookupRequestMessage *msg) {
        auto file = msg->file;
        WRENCH_INFO("CSS::processStorageLookupMessage(): For file %s", file->getID().c_str());

        if (this->file_location_mapping.find(file) == this->file_location_mapping.end()) {
            WRENCH_INFO("CSS::processStorageLookupMessage(): File %s is not known by this CompoundStorageService", file->getID().c_str());

            msg->answer_commport->dputMessage(
                    new CompoundStorageLookupAnswerMessage(
                            std::vector<std::shared_ptr<FileLocation>>(),
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));
        } else {
            auto mapped_locations = this->file_location_mapping[file];
            WRENCH_INFO("CSS::processStorageLookupMessage(): File %s is known by this CompoundStorageService", file->getID().c_str());
            for (const auto &loc: mapped_locations) {
                WRENCH_DEBUG("CSS::processStorageLookupMessage(): File stripe %s is known by this CompoundStorageService and associated to storage service %s",
                             loc->getFile()->getID().c_str(),
                             loc->getStorageService()->getName().c_str());
            }

            msg->answer_commport->dputMessage(
                    new CompoundStorageLookupAnswerMessage(
                            mapped_locations,
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));
        }

        return true;
    }

    /**
     * @brief Lookup for a DataFile in the internal file mapping of the CompoundStorageService (a simplified FileRegistry)
     *
     * @param file: the file of interest
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     *
     * @return A vector of shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or empty vector if it's not.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupFileLocation(const std::shared_ptr<DataFile> &file, S4U_CommPort *answer_commport) {
        WRENCH_INFO("CSS::lookupFileLocation() - DataFile + CommPort");

        this->commport->putMessage(
                new CompoundStorageLookupRequestMessage(
                        answer_commport,
                        file,
                        this->getMessagePayloadValue(
                                CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        auto msg = answer_commport->getMessage<CompoundStorageLookupAnswerMessage>(this->network_timeout, "CSS::lookupFileLocation(): Received a totally");

        return msg->locations;
    }

    /**
     *  @brief Lookup for a FileLocation (using its internal DataFile) in the internal file mapping of the CompoundStorageService
     *         (a simplified FileRegistry)
     *
     *  @param location: the location of interest
     *
     *  @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or nullptr if it's not.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupFileLocation(const std::shared_ptr<FileLocation> &location) {
        WRENCH_INFO("CSS::lookupFileLocation() - FileLocation");
        auto temp_commport = S4U_CommPort::getTemporaryCommPort();

        auto locations = this->lookupFileLocation(location->getFile(), temp_commport);

        S4U_CommPort::retireTemporaryCommPort(temp_commport);

        return locations;
    }

    /**
     *  @brief Lookup for a DataFile in the internal file mapping of the CompoundStorageService, and if it is not found,
     *         try to allocate the file on one of the underlying storage services, using the user-provided 'storage_selection'
     *         callback.
     *
     *  @param file: the file of interest
     *  @param stripe_count: number of stripes required for the given file (overrides any global setting)
     *  @param answer_commport: the answer commport to which the reply from the server should be sent
     *
     *  @return A vector of shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or could be allocated
     *          or empty vector if it's not / could not be allocated.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<DataFile>& file, unsigned int stripe_count, S4U_CommPort *answer_commport) {
        WRENCH_INFO("CSS::lookupOrDesignateStorageService() - DataFile + commport");

        this->commport->putMessage(new CompoundStorageAllocationRequestMessage(
                answer_commport,
                file,
                stripe_count,
                this->getMessagePayloadValue(
                        CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        auto msg = answer_commport->getMessage<CompoundStorageAllocationAnswerMessage>(this->network_timeout, "StorageService::writeFile(): Received a totally");
        return msg->locations;
    }

    /**
     *  @brief Lookup for a FileLocation (using its internal DataFile) in the internal file mapping of the CompoundStorageService,
     *         and if it is not found, try to allocate the file on one of the underlying storage services, using the user-provided
     *         'storage_selection' callback.
     *
     *  @param location: the location of interest
     *
     *  @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or could be allocated
     *          or nullptr if it's not.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<FileLocation>& location) {
        auto temp_commport = S4U_CommPort::getTemporaryCommPort();

        auto locations = this->lookupOrDesignateStorageService(location->getFile(), 0, temp_commport);

        S4U_CommPort::retireTemporaryCommPort(temp_commport);

        return locations;
    }

    /**
     *  @brief Lookup for a FileLocation (using its internal DataFile) in the internal file mapping of the CompoundStorageService,
     *         and if it is not found, try to allocate the file on one of the underlying storage services, using the user-provided
     *         'storage_selection' callback.
     *
     *  @param location: the location of interest
     *  @param stripe_count: number of stripes required for the given file (overrides any global setting)
     *
     *  @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or could be allocated
     *          or nullptr if it's not.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<FileLocation>& location, unsigned int stripe_count) {
        auto temp_commport = S4U_CommPort::getTemporaryCommPort();

        auto locations = this->lookupOrDesignateStorageService(location->getFile(), stripe_count, temp_commport);

        S4U_CommPort::retireTemporaryCommPort(temp_commport);

        return locations;
    }

    /**
     * @brief Delete a file on the storage service
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location to delete
     * @param wait_for_answer: whether this call should
     */
    void CompoundStorageService::deleteFile(S4U_CommPort *answer_commport,
                                            const std::shared_ptr<FileLocation> &location,
                                            bool wait_for_answer) {
        WRENCH_INFO("CSS::deleteFile(): Starting for file %s", location->getFile()->getID().c_str());

        if (!answer_commport or !location) {
            throw std::invalid_argument("CSS::deleteFile(): Invalid nullptr arguments");
        }
        if (location->isScratch()) {
            throw std::invalid_argument("CSS::deleteFile(): Cannot be called on a SCRATCH location");
        }

        auto designated_locations = this->lookupFileLocation(location);
        if (designated_locations.empty()) {
            throw ExecutionException(std::make_shared<FileNotFound>(location));
        }

        // this->traceInternalStorageUse(IOAction::DeleteStart, designated_locations);

        // Send a message to the storage service's daemon
        for (const auto &loc: designated_locations) {
            WRENCH_DEBUG("CSS:deleteFile Issuing delete message to SSS %s", loc->getStorageService()->getName().c_str());

            // assertServiceIsUp(loc->getStorageService());

            loc->getStorageService()->commport->putMessage(new StorageServiceFileDeleteRequestMessage(
                    answer_commport,
                    loc,
                    this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

            if (wait_for_answer) {
                std::unique_ptr<SimulationMessage> message = nullptr;

                auto msg = answer_commport->getMessage<StorageServiceFileDeleteAnswerMessage>(this->network_timeout, "StorageService::deleteFile():");
                if (!msg->success) {
                    throw ExecutionException(std::move(msg->failure_cause));
                }
            }
        }

        // Clean up local map
        this->file_location_mapping.erase(location->getFile());
        this->partial_io_stripe_index.erase(location->getFile());

        // Collect traces
        /*
        wrench::AllocationTrace trace;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = designated_locations;
        trace.act = IOAction::DeleteEnd;
        this->delete_traces[location->getFile()->getID()] = trace;
        */

        // this->traceInternalStorageUse(IOAction::DeleteEnd, designated_locations);
        WRENCH_INFO("CSS::deleteFile done for file %s", location->getFile()->getID().c_str());
    }

    /**
     * @brief Asks the storage service whether it holds a file
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location to lookup
     *
     * @return true if the file is present, false otherwise
     */
    bool CompoundStorageService::lookupFile(S4U_CommPort *answer_commport,
                                            const std::shared_ptr<FileLocation> &location) {
        WRENCH_INFO("CSS::lookupFile(): Lookup for file %s", location->getFile()->getID().c_str());

        if (!answer_commport or !location) {
            throw std::invalid_argument("CSS::lookupFile(): Invalid nullptr arguments");
        }

        auto file_parts = this->lookupFileLocation(location);
        if (file_parts.empty()) {
            WRENCH_DEBUG("CSS::lookupFile(): CSS doesn't know the file at location %s - %s",
                         location->getDirectoryPath().c_str(), location->getFile()->getID().c_str());
            return false;
        }

        bool available = true;

        // Send a message to the storage service's daemon
        for (const auto &loc: file_parts) {
            assertServiceIsUp(loc->getStorageService());

            loc->getStorageService()->commport->putMessage(new StorageServiceFileLookupRequestMessage(
                    answer_commport,
                    location,
                    this->getMessagePayloadValue(
                            StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

            // Wait for a reply
            auto msg = answer_commport->getMessage<StorageServiceFileLookupAnswerMessage>(this->network_timeout, "StorageService::lookupFile():");
            available &= msg->file_is_available;
        }

        return available;
    }

    /**
     * @brief Synchronously copy a file
     *
     * @param src_location: the source location
     * @param dst_location: the destination location
     */
    void CompoundStorageService::copyFile(const std::shared_ptr<FileLocation> &src_location,
                                          const std::shared_ptr<FileLocation> &dst_location) {
        WRENCH_INFO("CSS::copyFile() at %f: Copy %s between %s and %s",
                    S4U_Simulation::getClock(),
                    src_location->getFile()->getID().c_str(),
                    src_location->getStorageService()->getName().c_str(),
                    dst_location->getStorageService()->getName().c_str());

        // Check if source file is on a CSS and whether or not it is stripped across many locations
        bool src_is_css = false;
        if (std::dynamic_pointer_cast<CompoundStorageService>(src_location->getStorageService())) {
            src_is_css = true;
        }

        // Check if destination file is on a CSS and whether or not it is stripped across many locations
        bool dst_is_css = false;
        if (std::dynamic_pointer_cast<CompoundStorageService>(dst_location->getStorageService())) {
            dst_is_css = true;
        }

        if (src_is_css and !dst_is_css) {
            WRENCH_INFO("CSS::copyFile(): src_location is on a CSS");
            auto src_css = std::dynamic_pointer_cast<CompoundStorageService>(src_location->getStorageService());
            src_css->copyFileIamSource(src_location, dst_location);
        } else if (!src_is_css and dst_is_css) {
            WRENCH_INFO("CSS::copyFile(): dst_location is on a CSS");
            auto dst_css = std::dynamic_pointer_cast<CompoundStorageService>(dst_location->getStorageService());
            dst_css->copyFileIamDestination(src_location, dst_location);
        } else if (src_is_css and dst_is_css) {
            // Case where both src and dst are CSS
            WRENCH_INFO("CSS::copyFile(): src_location AND dst_location are on a CSS");
            throw std::invalid_argument("CompoundStorageService::copyFile() copy from CSS to CSS not yet handled");
        } else {
            WRENCH_WARN("CSS::copyFile(): called but neither src or dst is a CSS");
            throw std::invalid_argument("CompoundStorageService::copyFile() called but neither src or dst is a CSS");
        }
    }

    /**
     * @brief Copy file from css to a simple storage service (file might be stripped within the CSS, but should be
     *        reassembled on the SSS)
     *
     * @param src_location: the source location
     * @param dst_location: the destination location
     *
     */
    void CompoundStorageService::copyFileIamSource(const std::shared_ptr<FileLocation> &src_location,
                                                   const std::shared_ptr<FileLocation> &dst_location) {
        WRENCH_INFO("CSS::copyFileIamSource() - Starting at %f", S4U_Simulation::getClock());

        std::vector<std::shared_ptr<FileLocation>> src_parts = {};
        std::vector<std::shared_ptr<FileLocation>> dst_parts = {};

        // Find source file or parts of source file from within internal sss of the CSS (source file must be known)
        src_parts = this->lookupFileLocation(src_location);
        if (src_parts.empty()) {
            throw ExecutionException(std::make_shared<FileNotFound>(src_location));
        }

        WRENCH_INFO("CSS::copyFileIamSource(): Source file has %zu known part(s)", src_parts.size());
        if (src_parts.size() > 1) {
            for (const auto &src_part: src_parts) {
                dst_parts.push_back(
                        FileLocation::LOCATION(dst_location->getStorageService(), dst_location->getDirectoryPath(), src_part->getFile()));
            }
        } else {
            dst_parts.push_back(dst_location);// no stripping, dst location doesn't have to change
        }

        // Now run the copy(ies) between the source(s) and the destination(s)
        auto copy_idx = 0;
        int total_parts = src_parts.size();// = dst_parts.size()

        auto tmp_commport = S4U_CommPort::getTemporaryCommPort();

        while (copy_idx < total_parts) {
            WRENCH_DEBUG("CSS::copyFileIamSource(): Running StorageService::copyFile for part %i", copy_idx);

            // assertServiceIsUp(src_parts[copy_idx]->getStorageService());
            // assertServiceIsUp(dst_parts[copy_idx]->getStorageService());

            auto file = src_parts[copy_idx]->getFile();
            bool src_is_bufferized = src_parts[copy_idx]->getStorageService()->isBufferized();
            bool src_is_non_bufferized = not src_is_bufferized;
            bool dst_is_bufferized = dst_parts[copy_idx]->getStorageService()->isBufferized();
            bool dst_is_non_bufferized = not dst_is_bufferized;

            S4U_CommPort *commport_to_contact;
            if (dst_is_non_bufferized) {
                commport_to_contact = dst_parts[copy_idx]->getStorageService()->commport;
            } else if (src_is_non_bufferized) {
                commport_to_contact = src_parts[copy_idx]->getStorageService()->commport;
            } else {
                commport_to_contact = dst_parts[copy_idx]->getStorageService()->commport;
            }

            this->simulation_->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                    src_parts[copy_idx],
                                                                    dst_parts[copy_idx]);
            commport_to_contact->dputMessage(
                    new StorageServiceFileCopyRequestMessage(
                            tmp_commport,
                            src_parts[copy_idx],
                            dst_parts[copy_idx],
                            dst_parts[copy_idx]->getStorageService()->getMessagePayloadValue(
                                    StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

            copy_idx++;
        }

        WRENCH_INFO("CSS::copyFileIamSource(): Copy/ies started");

        // Wait for all replies
        auto rcv = 0;
        while (rcv < total_parts) {
            auto msg = tmp_commport->getMessage<StorageServiceFileCopyAnswerMessage>();
            rcv += 1;

            if (msg->failure_cause) {
                WRENCH_WARN("%s", msg->failure_cause->toString().c_str());
                throw ExecutionException(std::move(msg->failure_cause));
            }
        }
        S4U_CommPort::retireTemporaryCommPort(tmp_commport);
        WRENCH_INFO("CSS::copyFileIamSource(): Copy/ies started (answers received)");

        // Cleanup - Once all the copies are made, we need to delete parts on destination and merge into a single file
        if (total_parts > 1) {
            WRENCH_DEBUG("CSS::copyFileIamSource(): Cleaning up file parts on destination to merge as a single file");
            auto dest_storage_svc = dst_location->getStorageService();
            for (const auto &part: dst_parts) {
                dest_storage_svc->deleteFileAtLocation(part);
            }
            dest_storage_svc->createFileAtLocation(FileLocation::LOCATION(dest_storage_svc,
                                                                          dst_location->getDirectoryPath(), dst_location->getFile()));
            WRENCH_DEBUG("CSS::copyFileIamSource(): All parts on destination replaced by a single file");
        }

        // Collect traces
        /*
        wrench::AllocationTrace trace;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = src_parts;
        trace.act = IOAction::CopyToEnd;
        this->copy_traces[src_location->getFile()->getID()] = trace;
        */

        WRENCH_INFO("CSS::copyFileIamSource(): Done");
    }

    /**
     * @brief Copy file from a SimpleStorageService to a CSS. Src file cannot be stripped, but copy might
     *          result in stripped file on CSS.
     * @param src_location: the source location
     * @param dst_location: the destination location
     *
     */
    void CompoundStorageService::copyFileIamDestination(const std::shared_ptr<FileLocation> &src_location,
                                                        const std::shared_ptr<FileLocation> &dst_location) {
        WRENCH_DEBUG("CSS::copyFileIamDestination() - Starting at %f", S4U_Simulation::getClock());

        // this->traceInternalStorageUse(IOAction::CopyToStart);

        std::vector<std::shared_ptr<FileLocation>> src_parts = {};
        std::vector<std::shared_ptr<FileLocation>> dst_parts = {};

        // Find one or many SSS location(s) for the destination file (depending on whether the original file needs stripping or not)*
        dst_parts = this->lookupOrDesignateStorageService(dst_location);
        if (dst_parts.empty()) {
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(dst_location->getFile(), this->getSharedPtr<CompoundStorageService>()));
        }

        // this->traceInternalStorageUse(IOAction::CopyToStart, dst_parts);
        WRENCH_INFO("CSS::copyFileIamDestination(): Destination file will be written as %zu stripe(s) / file_part(s)", dst_parts.size());
        if (dst_parts.size() > 1) {
            for (const auto &dst_part: dst_parts) {
                auto part_size = dst_part->getFile()->getSize();
                dst_part->getFile()->setSize(0);     // make our datafile act as a reference / link
                StorageService::createFileAtLocation(// create link on source storage service
                        FileLocation::LOCATION(
                                src_location->getStorageService(),
                                src_location->getDirectoryPath(),
                                dst_part->getFile()));
                // Prepare for copy once again
                dst_part->getFile()->setSize(part_size);
                src_parts.push_back(
                        FileLocation::LOCATION(src_location->getStorageService(), src_location->getDirectoryPath(), dst_part->getFile()));
            }
            WRENCH_INFO("CSS::copyFileIamDestination(): %zu stripe(s) / file_part(s) created from source file", src_parts.size());
        } else {
            src_parts.push_back(src_location);// no stripping, src location doesn't have to change
        }

        // Now run the copy(ies) between the source(s) and the destination(s)
        auto copy_idx = 0;
        int total_parts = src_parts.size();// = dst_parts.size()

        auto tmp_commport = S4U_CommPort::getTemporaryCommPort();

        while (copy_idx < total_parts) {
            WRENCH_DEBUG("CSS::copyFileIamDestination(): Running StorageService::copyFile for part %i", copy_idx);
            WRENCH_DEBUG("CSS::copyFileIamDestination(): SRC= %s - size %llu at %s on  %s%s",
                         src_parts[copy_idx]->getFile()->getID().c_str(),
                         src_parts[copy_idx]->getFile()->getSize(),
                         src_parts[copy_idx]->getDirectoryPath().c_str(),
                         src_parts[copy_idx]->getStorageService()->getHostname().c_str(),
                         src_parts[copy_idx]->getStorageService()->getBaseRootPath().c_str());
            WRENCH_DEBUG("CSS::copyFileIamDestination(): DST= %s - size %llu at %s on  %s%s",
                         dst_parts[copy_idx]->getFile()->getID().c_str(),
                         dst_parts[copy_idx]->getFile()->getSize(),
                         dst_parts[copy_idx]->getDirectoryPath().c_str(),
                         dst_parts[copy_idx]->getStorageService()->getHostname().c_str(),
                         dst_parts[copy_idx]->getStorageService()->getBaseRootPath().c_str());

            // Useful ?
            // assertServiceIsUp(src_parts[copy_idx]->getStorageService());
            // assertServiceIsUp(dst_parts[copy_idx]->getStorageService());

            auto file = src_parts[copy_idx]->getFile();
            bool src_is_bufferized = src_parts[copy_idx]->getStorageService()->isBufferized();
            bool src_is_non_bufferized = not src_is_bufferized;
            bool dst_is_bufferized = dst_parts[copy_idx]->getStorageService()->isBufferized();
            bool dst_is_non_bufferized = not dst_is_bufferized;

            S4U_CommPort *commport_to_contact;
            if (dst_is_non_bufferized) {
                commport_to_contact = dst_parts[copy_idx]->getStorageService()->commport;
            } else if (src_is_non_bufferized) {
                commport_to_contact = src_parts[copy_idx]->getStorageService()->commport;
            } else {
                commport_to_contact = dst_parts[copy_idx]->getStorageService()->commport;
            }

            this->simulation_->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                    src_parts[copy_idx],
                                                                    dst_parts[copy_idx]);

            // That's quite dirty, but : space is reserved during the call to lookupOrDesignateFileLocation, so that
            // we keep track of future space usage on various storage nodes while allocating multiple chunks of a given
            // file. So right before we actually start the copy, we unreserve space.
            dst_parts[copy_idx]->getStorageService()->unreserveSpace(dst_parts[copy_idx]);
            commport_to_contact->dputMessage(
                    new StorageServiceFileCopyRequestMessage(
                            tmp_commport,
                            src_parts[copy_idx],
                            dst_parts[copy_idx],
                            dst_parts[copy_idx]->getStorageService()->getMessagePayloadValue(
                                    StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

            copy_idx++;
        }

        WRENCH_INFO("CSS::copyFileIamDestination(): Copy/ies started");

        // Wait for all replies
        auto rcv = 0;
        while (rcv < total_parts) {
            auto msg = tmp_commport->getMessage<StorageServiceFileCopyAnswerMessage>();
            rcv += 1;

            if (msg->failure_cause) {
                WRENCH_WARN("%s", msg->failure_cause->toString().c_str());
                throw ExecutionException(std::move(msg->failure_cause));
            }
        }
        S4U_CommPort::retireTemporaryCommPort(tmp_commport);

        WRENCH_INFO("CSS::copyFileIamDestination(): Copy/ies started (answers received)");

        // Once copy is done, remove links (only if source file was stripped)
        if (total_parts > 1) {
            WRENCH_DEBUG("CSS::copyFileIamDestination(): Cleaning up file parts from source");
            for (const auto &src_part: src_parts) {
                auto part_size = src_part->getFile()->getSize();
                src_part->getFile()->setSize(0);// make our datafile a link once again
                WRENCH_DEBUG("CSS::copyFileIamDestination(): Deleting part %s", src_part->getFile()->getID().c_str());
                StorageService::deleteFileAtLocation(src_part);
                src_part->getFile()->setSize(part_size);
            }
            WRENCH_INFO("CSS::copyFileIamDestination(): Source parts deleted");
        }

        // Collect traces
        /*
        wrench::AllocationTrace trace;
        trace.act = IOAction::WriteEnd;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = dst_parts;
        this->copy_traces[dst_location->getFile()->getID()] = trace;
        */

        // this->traceInternalStorageUse(IOAction::CopyToEnd, dst_parts);
        WRENCH_INFO("CSS::copyFileIamDestination(): Done (cleanup + tracing)");
    }

    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location
     * @param num_bytes_to_write: number of bytes to write to the file
     * @param wait_for_answer: whether to wait for the answer
     *
     */
    void CompoundStorageService::writeFile(S4U_CommPort *answer_commport /*unused*/,
                                           const std::shared_ptr<FileLocation> &location,
                                           sg_size_t num_bytes_to_write,
                                           bool wait_for_answer) {
        WRENCH_INFO("CSS::writeFile(): Writing  %llu to file %s - starting at %f", num_bytes_to_write, location->getFile()->getID().c_str(), S4U_Simulation::getClock());

        if (location == nullptr) {
            throw std::invalid_argument("CSS::writeFile(): Invalid arguments (location is a nullptr)");
        }

        this->assertServiceIsUp();

        // Find the file, or allocate file/parts of file onto known SSS
        auto designated_locations = this->lookupOrDesignateStorageService(location);
        if (designated_locations.empty()) {
            WRENCH_WARN("CSS::writeFile(): 'designated_locations' vector empty (error or lack of space from the allocator point of view)");
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(location->getFile(), this->getSharedPtr<CompoundStorageService>()));
        }

        // If num_bytes_to_write > the entire file size, we have a big problem (it should almost always be way smaller, as it represents the bytes only in a subset of all file chunks)
        if (num_bytes_to_write > location->getFile()->getSize()) {
            WRENCH_WARN("CSS::writeFile(): 'num_bytes_to_write' is larger than actual file size");
            throw std::invalid_argument("'num_bytes_to_write' is larger than actual file size");
        }

        // Identify, for current main file, where to start writing (maybe at the beginning, maybe somewhere in the file, indicated by a chunk index)
        WRENCH_INFO("CSS::writeFile(): looking for first and last stripes to write for %s", location->getFile()->getID().c_str());
        auto locations_start = this->partial_io_stripe_index[location->getFile()];
        if (locations_start == designated_locations.size()) {
            locations_start = 0;
            this->partial_io_stripe_index[location->getFile()] = 0;
        }

        // Find where to stop writing : starting from locations_start, we keep iterating up the vector until we reach the end of the chunks or the amount of
        // bytes to write is depleted
        auto stripe_count = designated_locations.size();
        while (num_bytes_to_write > 0 and locations_start != stripe_count) {
            WRENCH_INFO("  - total_bytes_to_write = %llu and current file (%s) is %llu bytes",
                        num_bytes_to_write,
                        designated_locations[locations_start]->getFile()->getID().c_str(),
                        designated_locations[locations_start]->getFile()->getSize());
            num_bytes_to_write -= designated_locations[locations_start]->getFile()->getSize();
            locations_start++;
        }
        locations_start--;// back up one stripe because when writing eg. 1 stripe, we want to write from stripe X to stripe X, not stripe X to X+1

        // Writes are now going to take place on these locations only
        WRENCH_INFO("CSS::writeFile(): Writing file %s from part %s to part %s",
                    location->getFile()->getID().c_str(),
                    designated_locations[this->partial_io_stripe_index[location->getFile()]]->getFile()->getID().c_str(),
                    designated_locations[locations_start]->getFile()->getID().c_str());

        // Prepare a vector containing all file chunks that will need to be written (one per action)
        std::vector<std::shared_ptr<wrench::FileLocation>> designated_locations_subset(
                designated_locations.begin() + this->partial_io_stripe_index[location->getFile()],
                designated_locations.begin() + (++locations_start));
        // UPDATE the next starting location for writing file chunks
        this->partial_io_stripe_index[location->getFile()] = locations_start;

        // this->traceInternalStorageUse(IOAction::WriteStart, designated_locations_subset);
        WRENCH_INFO("CSS::writeFile(): Destination file %s has %zu stripes, and for now we'll be writing on %zu of these stripes",
                    location->getFile()->getID().c_str(),
                    designated_locations.size(),
                    designated_locations_subset.size());

        // Contact every SimpleStorageService that we want to use, and request a FileWrite
        auto recv_commport = S4U_CommPort::getTemporaryCommPort();
        unsigned int request_count = 0;
        WRENCH_INFO("CSS::writeFile(): Using commport created : %s", recv_commport->get_name().c_str());
        for (auto &dloc: designated_locations_subset) {
            WRENCH_INFO("CSS::writeFile(): Sending full write request %d on file %s (<%llu> b) to %s",
                        request_count, dloc->getFile()->getID().c_str(), dloc->getFile()->getSize(), dloc->getStorageService()->getName().c_str());

            //TODO: THIS IS UGLY, PERHAPS WOULD BE BETTER TO THINK UP ANOTHER "RESERVE" SCHEME...
            dloc->getStorageService()->unreserveSpace(dloc);

            dloc->getStorageService()->commport->dputMessage(
                    new StorageServiceFileWriteRequestMessage(
                            recv_commport,
                            simgrid::s4u::this_actor::get_host(),
                            dloc,
                            dloc->getFile()->getSize(),
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
            request_count++;
        }

        WRENCH_INFO("CSS::writeFile(): %i write requests sent", request_count);

        std::vector<std::unique_ptr<wrench::StorageServiceFileWriteAnswerMessage>> messages = {};
        unsigned int recv = 0;
        while (recv < request_count) {
            // Wait for answer to current request
            auto msg = recv_commport->getMessage<StorageServiceFileWriteAnswerMessage>(this->network_timeout, "CSS::writeFile(): ");
            if (not msg->success) {
                WRENCH_WARN("CSS::fileWrite(): Network timeout while waiting for and answer %d", recv);
                throw ExecutionException(msg->failure_cause);
            }
            messages.push_back(std::move(msg));
            recv++;
        }

        WRENCH_INFO("CSS::writeFile(): %u FileWriteRequests sent and validated", request_count);

        for (const auto &msg: messages) {
            // Update buffer size according to which storage service actually answered.
            auto buffer_size = msg->buffer_size;

            if (buffer_size >= 1) {
                auto file = location->getFile();
                for (auto const &dwmb: msg->data_write_commport_and_bytes) {
                    // Bufferized
                    sg_size_t remaining = dwmb.second;
                    while (remaining - buffer_size > 0) {
                        dwmb.first->dputMessage(new StorageServiceFileContentChunkMessage(
                                file, buffer_size, false));
                        remaining -= buffer_size;
                    }
                    dwmb.first->dputMessage(new StorageServiceFileContentChunkMessage(
                            file, remaining, true));
                }
            }
        }

        WRENCH_INFO("CSS::writeFile(): Waiting for final acks");
        for (const auto &mailbox_msg: messages) {
            recv_commport->getMessage<StorageServiceAckMessage>("CSS::writeFile(): ");
        }
        S4U_CommPort::retireTemporaryCommPort(recv_commport);

        // Collect traces
        /*
        wrench::AllocationTrace trace;
        trace.act = IOAction::WriteEnd;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = designated_locations;
        this->write_traces[location->getFile()->getID()] = trace;
        */

        // this->traceInternalStorageUse(IOAction::WriteEnd, designated_locations_subset);

        for (const auto &loc: designated_locations_subset) {
            WRENCH_DEBUG("CSS::writeFile(): For location %s, free space = %llu", loc->getStorageService()->getName().c_str(), loc->getStorageService()->getTotalFreeSpace());
        }

        WRENCH_INFO("CSS::writeFile(): All writes done and ack");
    }

    /**
     * @brief Read a file from the storage service
     *
     * @param answer_commport: the answer commport to which the reply from the server should be sent
     * @param location: the location
     * @param num_bytes: the number of bytes to read
     * @param wait_for_answer: whether to wait for the answer
     */
    void CompoundStorageService::readFile(S4U_CommPort *answer_commport /*unused*/,
                                          const std::shared_ptr<FileLocation> &location,
                                          sg_size_t num_bytes,
                                          bool wait_for_answer) {
        WRENCH_INFO("CSS::readFile(): Reading file %s - starting at %f", location->getFile()->getID().c_str(), S4U_Simulation::getClock());

        if (!answer_commport or !location) {
            throw std::invalid_argument("StorageService::readFile(): Invalid nullptr arguments");
        }

        this->assertServiceIsUp();

        auto designated_locations = this->lookupFileLocation(location);
        if (designated_locations.empty()) {
            throw ExecutionException(std::make_shared<FileNotFound>(location));
        }

        WRENCH_INFO("CSS::readFile(): looking for first and last stripes to read for %s", location->getFile()->getID().c_str());
        auto locations_start = this->partial_io_stripe_index[location->getFile()];
        if (locations_start == designated_locations.size()) {
            locations_start = 0;
            this->partial_io_stripe_index[location->getFile()] = 0;
        }

        /*  Find where to stop reading : starting from locations_start, we keep iterating up the vector until we reach the end or the amount of
            bytes to read is depleted
        */
        auto stripe_count = designated_locations.size();
        while (num_bytes > 0 and locations_start != stripe_count) {
            WRENCH_INFO("CSS::readFile():  - num_bytes = %llu and current file (%s) is %llu bytes",
                        num_bytes,
                        designated_locations[locations_start]->getFile()->getID().c_str(),
                        designated_locations[locations_start]->getFile()->getSize());
            num_bytes -= designated_locations[locations_start]->getFile()->getSize();
            locations_start++;
        }
        locations_start--;

        // Writes are now going to take place on these locations only
        WRENCH_INFO("CSS::readFile(): Reading file %s from part %s to part %s",
                    location->getFile()->getID().c_str(),
                    designated_locations[this->partial_io_stripe_index[location->getFile()]]->getFile()->getID().c_str(),
                    designated_locations[locations_start]->getFile()->getID().c_str());

        std::vector<std::shared_ptr<wrench::FileLocation>> designated_locations_subset(
                designated_locations.begin() + this->partial_io_stripe_index[location->getFile()],
                designated_locations.begin() + (++locations_start));
        // UPDATE the progress on reading the file stripes
        this->partial_io_stripe_index[location->getFile()] = locations_start;

        // Contact every SSS
        auto recv_commport = S4U_CommPort::getTemporaryCommPort();
        unsigned int request_count = 0;
        for (const auto &dloc: designated_locations_subset) {
            WRENCH_DEBUG("CSS::readFile(): Sending full read request %d on file %s (<%llu> b) to %s",
                         request_count, dloc->getFile()->getID().c_str(), dloc->getFile()->getSize(), dloc->getStorageService()->getName().c_str());

            dloc->getStorageService()->commport->dputMessage(
                    new StorageServiceFileReadRequestMessage(
                            recv_commport,
                            simgrid::s4u::this_actor::get_host(),
                            dloc,
                            dloc->getFile()->getSize(),// Note : not using num_bytes, because we know how many full stripes to write to amount to the same number of bytes
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
            request_count++;
        }

        WRENCH_DEBUG("CSS::readFile(): %i read requests sent", request_count);

        std::vector<std::unique_ptr<wrench::StorageServiceFileReadAnswerMessage>> messages = {};
        unsigned int recv = 0;
        while (recv < request_count) {
            // Wait for answer to current request
            auto msg = recv_commport->getMessage<StorageServiceFileReadAnswerMessage>(this->network_timeout, "CSS::readFile(): ");
            if (not msg->success) {
                throw ExecutionException(msg->failure_cause);
            }

            messages.push_back(std::move(msg));
            recv++;
        }

        WRENCH_DEBUG("CSS::readFile(): %u FileReadRequests sent and validated", request_count);

        for (const auto &msg: messages) {
            if (msg->buffer_size < 1) {
                // Non-Bufferized ; just wait for an ack for this message (note this may not be THE ack to this precise message, but it doesn't matter)
                recv_commport->getMessage<StorageServiceAckMessage>("CSS::readFile(): ");
            } else {
                unsigned long number_of_sources = msg->number_of_sources;

                // Otherwise, retrieve the file chunks until the last one is received
                // Noting that we have multiple sources
                unsigned long num_final_chunks_received = 0;
                while (true) {
                    std::shared_ptr<StorageServiceFileContentChunkMessage> file_content_chunk_msg = nullptr;
                    try {
                        file_content_chunk_msg = msg->commport_to_receive_the_file_content->getMessage<StorageServiceFileContentChunkMessage>("StorageService::readFile(): Received an");
                    } catch (...) {
                        S4U_CommPort::retireTemporaryCommPort(msg->commport_to_receive_the_file_content);
                        throw;
                    }
                    if (file_content_chunk_msg->last_chunk) {
                        num_final_chunks_received++;
                        if (num_final_chunks_received == msg->number_of_sources) {
                            break;
                        }
                    }
                }

                // S4U_CommPort::retireTemporaryCommPort(msg->commport_to_receive_the_file_content);

                // Waiting for all the final acks (same thing as for un buffered sources : we're waiting on N ack messages, not necessarily the ones corresponding to this exact message)
                for (unsigned long source = 0; source < number_of_sources; source++) {
                    recv_commport->getMessage<StorageServiceAckMessage>(this->network_timeout, "StorageService::readFile(): Received an");
                }
            }
        }

        S4U_CommPort::retireTemporaryCommPort(recv_commport);

        WRENCH_INFO("CSS::readFile(): All %i reads done and ack at %f)", request_count, S4U_Simulation::getClock());

        // Collect traces
        /*
        wrench::AllocationTrace trace;
        trace.ts = S4U_Simulation::getClock();
        trace.act = IOAction::ReadEnd;
        trace.internal_locations = designated_locations_subset;
        // this->read_traces[location->getFile()->getID()] = trace;
        */
    }

    /**
     * @brief Get the load (number of concurrent reads) on the storage service
     *        Not implemented yet for CompoundStorageService (is it needed?)
     * @return the load on the service (currently throws)
     */
    double CompoundStorageService::getLoad() {
        WRENCH_WARN("CSS::getLoad(): Not implemented");
        throw std::logic_error("CSS::getLoad(): Not implemented. "
                               "Call getLoad() on an underlying storage service instead");
    }

    /**
     * @brief Get the total space across all internal services known by the CompoundStorageService
     *
     * @return A number of bytes
     */
    sg_size_t CompoundStorageService::getTotalSpace() {
        // WRENCH_INFO("CompoundStorageService::getTotalSpace");
        sg_size_t free_space = 0.0;
        for (const auto &storage_server: this->storage_services) {
            for (const auto &service: storage_server.second) {
                free_space += service->getTotalSpace();
            }
        }
        return free_space;
    }

    /**
     * @brief Synchronously asks the storage services inside the compound storage service
     *        for their free space at all of their mount points
     *
     * @param path a path
     *
     * @return The free space in bytes at the path
     *
     */
    sg_size_t CompoundStorageService::getTotalFreeSpaceAtPath(const std::string &path) {
        WRENCH_DEBUG("CSS::getFreeSpace Forwarding request to internal services");

        sg_size_t free_space = 0;
        for (const auto &storage_server: this->storage_services) {
            for (const auto &service: storage_server.second) {
                free_space += service->getTotalFreeSpaceAtPath(path);
            }
        }
        return free_space;
    }

    /**
     *  @brief setIsScratch can't be used on a CompoundStorageService because it doesn't have any actual storage resources.
     *  @param is_scratch true or false
     */
    void CompoundStorageService::setIsScratch(bool is_scratch) {
        WRENCH_WARN("CSS::setScratch Forbidden because CompoundStorageService doesn't manage any storage resources itself");
        throw std::logic_error("CSS: can't be setup as a scratch space, it is only an abstraction layer.");
    }

    /**
     * @brief Return the set of all services accessible through this CompoundStorageService
     *
     * @return The set of known StorageServices.
     */
    std::map<std::string, std::vector<std::shared_ptr<wrench::StorageService>>> &CompoundStorageService::getAllServices() {
        WRENCH_DEBUG("CSS::getAllServices");
        return this->storage_services;
    }

    /**
     * @brief Get a file's last write date at a location (in zero simulated time)
     *
     * @param location: the location
     *
     * @return a date in seconds, or -1 if the file is not found
     */
    double CompoundStorageService::getFileLastWriteDate(const std::shared_ptr<FileLocation> &location) {
        if (location == nullptr) {
            throw std::invalid_argument("CSS::getFileLastWriteDate(): Invalid nullptr argument");
        }

        auto designated_locations = this->lookupFileLocation(location);
        if (designated_locations.empty()) {
            throw std::invalid_argument("CSS::getFileLastWriteDate(): File not known to the CompoundStorageService. Unable to forward to underlying StorageService");
        }

        for (const auto &location: designated_locations) {
            auto designated_storage_service = std::dynamic_pointer_cast<SimpleStorageService>(location->getStorageService());
            if (designated_storage_service)// In case of multiple file parts, return LastWriteDate from first part found (they should be all the same, or very close)
                return designated_storage_service->getFileLastWriteDate(location);
        }

        return -1;
    }

    /**
     * @brief Check (outside of simulation time) whether the storage service has a file
     *
     * @param location a location
     *
     * @return true if the file is present, false otherwise
     */
    bool CompoundStorageService::hasFile(const std::shared_ptr<FileLocation> &location) {
        auto file_location = this->lookupFileLocation(location);
        if (file_location.empty()) {
            WRENCH_INFO("CSS::hasFile(): File %s not found", location->getFile()->getID().c_str());
            return false;
        }

        // check internal path as well

        return true;
    }

    /**
     * @brief Generate a unique number
     *
     * @return a unique number
     */
    unsigned long CompoundStorageService::getNewUniqueNumber() {
        static unsigned long sequence_number = 0;
        return (sequence_number++);
    }

    /**
     * @brief Process a stop daemon request
     *
     * @param ack_commport: the commport to which the ack should be sent
     *
     * @return false if the daemon should terminate
     */
    bool CompoundStorageService::processStopDaemonRequest(S4U_CommPort *ack_commport) {
        try {
            ack_commport->putMessage(new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
                    CompoundStorageServiceMessagePayload::DAEMON_STOPPED_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            return false;
        }
        return false;
    }

    void CompoundStorageService::traceInternalStorageUse(IOAction action, const std::vector<std::shared_ptr<FileLocation>> &locations) {
        auto ts = S4U_Simulation::getClock();
        AllocationTrace trace;
        trace.act = action;
        trace.ts = ts;
        trace.parts_count = size(locations);

        if (locations.empty()) {
            trace.file_name = "nofile";

            for (const auto &storage: this->storage_services) {
                for (const auto &storage_service: storage.second) {
                    DiskUsage disk_usage;
                    disk_usage.service = storage_service;
                    disk_usage.free_space = storage_service->getTotalFreeSpaceZeroTime();
                    disk_usage.file_count = storage_service->getTotalFilesZeroTime();
                    trace.disk_usage.push_back(disk_usage);
                }
            }

        } else {
            trace.file_name = locations.begin()->get()->getFile()->getID();

            std::set<std::string> known_services;

            for (const auto &location: locations) {
                auto storage_service = location->getStorageService();
                if (known_services.find(storage_service->getName()) == known_services.end()) {
                    DiskUsage disk_usage;
                    disk_usage.service = storage_service;
                    disk_usage.free_space = storage_service->getTotalFreeSpaceZeroTime();
                    disk_usage.file_count = storage_service->getTotalFilesZeroTime();
                    trace.disk_usage.push_back(disk_usage);

                    if (size(trace.disk_usage) == this->total_nb_storage_services) {
                        break;
                    }
                }
            }
        }

        this->internal_storage_use.push_back(std::make_pair(ts, trace));
    }
};// namespace wrench

#include <wrench/services/storage/compound/CompoundStorageService.h>

#include "wrench/services/storage/compound/CompoundStorageServiceMessage.h"
#include <wrench/services/ServiceMessage.h>
#include <wrench/services/storage/simple/SimpleStorageService.h>
#include "wrench/services/storage/StorageServiceMessage.h"
#include <wrench/services/storage/StorageServiceMessagePayload.h>
#include <wrench/simgrid_S4U_util/S4U_Mailbox.h>
#include <wrench/failure_causes/FileNotFound.h>
#include <wrench/failure_causes/StorageServiceNotEnoughSpace.h>
#include <wrench/failure_causes/NotAllowed.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>

WRENCH_LOG_CATEGORY(wrench_core_compound_storage_system,
                    "Log category for Compound Storage Service");

namespace wrench {

    /** 
     *  @brief Default StorageSelectionStrategyCallback: strategy used by the CompoundStorageService 
     *         when no strategy is provided at instanciation. By default, it returns a nullptr, which 
     *         trigger any request message processing function in CompoundStorageServer to answer negatively.
     *
     *  @param file: the file
     *  @param resources: the set of potential storage services
     *  @param mapping: helper data structure to find the relevant location for a file
     *  @param previous_allocations: previous allocations
     * 
     *  @return nullptr (instead of a valid FileLocation)
    */
    std::shared_ptr<FileLocation> nullptrStorageServiceSelection(
            const std::shared_ptr<DataFile> &file,
            const std::map<std::string, std::vector<std::shared_ptr<StorageService>>> &resources,
            const std::map<std::shared_ptr<DataFile>, std::vector<std::shared_ptr<FileLocation>>> &mapping,
            const std::vector<std::shared_ptr<FileLocation>> &previous_allocations) {
        return nullptr;
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
                                                   WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : CompoundStorageService(hostname, storage_services, nullptrStorageServiceSelection, false,
                                                                                                                                       property_list, messagepayload_list, "_" + std::to_string(getNewUniqueNumber())){};


    /** 
     *  @brief Constructor for the case where the user provides a callback (StorageSelectionStrategyCallback) 
     *         which will be used by the CompoundStorageService any time it receives a file write or file copy 
     *         request, in order to determine which underlying StorageService to use for the (potentially) new
     *         file in the request. 
     *         Note that nothing prevents the user from also intercepting some actions (see use case for other 
     *         constructor), but resulting behaviour is undefined.
     *  @param hostname: the name of the host on which this service will run
     *  @param storage_services: subordinate storage services
     *  @param storage_selection: the storage selection strategy callback
     *  @param property_list: the configurable properties
     *  @param messagepayload_list: the configurable message payloads
     */
    CompoundStorageService::CompoundStorageService(const std::string &hostname,
                                                   std::set<std::shared_ptr<StorageService>> storage_services,
                                                   StorageSelectionStrategyCallback storage_selection,
                                                   WRENCH_PROPERTY_COLLECTION_TYPE property_list,
                                                   WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list) : CompoundStorageService(hostname, storage_services, storage_selection, true, property_list, messagepayload_list,
                                                                                                                                       "_" + std::to_string(getNewUniqueNumber())){};


    /**
     *  @brief Constructor
     *  @param hostname: the name of the host on which this service will run
     *  @param storage_services: subordinate storage services
     *  @param storage_selection: the storage selection strategy callback
     *  @param storage_selection_user_provided: whether the storage selection is user-provided
     *  @param property_list: the configurable properties
     *  @param messagepayload_list: the configurable message payloads
     *  @param suffix: the suffix to add to the service name
     */
    CompoundStorageService::CompoundStorageService(
            const std::string &hostname,
            std::set<std::shared_ptr<StorageService>> storage_services,
            StorageSelectionStrategyCallback storage_selection,
            bool storage_selection_user_provided,
            WRENCH_PROPERTY_COLLECTION_TYPE property_list,
            WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE messagepayload_list,
            const std::string &suffix) : StorageService(hostname,
                                                        "compound_storage" + suffix) {


        this->setProperties(this->default_property_values, std::move(property_list));
        this->setMessagePayloads(this->default_messagepayload_values, std::move(messagepayload_list));

        if (storage_services.empty()) {
            throw std::invalid_argument("Got an empty list of StorageServices for CompoundStorageService."
                                        "Must specify at least one valid StorageService");
        }

        if (std::any_of(storage_services.begin(), storage_services.end(), [](const auto &elem) { return elem == NULL; })) {
            throw std::invalid_argument("One of the StorageServices provided is not initialized");
        }

        /* For now, we do not allow storage services that are simple with more than one mount point */
        if (std::any_of(storage_services.begin(), storage_services.end(), [](const auto &elem) {
                auto sss = std::dynamic_pointer_cast<SimpleStorageService>(elem);
                return sss->hasMultipleMountPoints();
            })) {
            throw std::invalid_argument("One of the SimpleStorageServices provided has more than one mount point. "
                                        "In the current state of the implementation this is currently not allowed");
        }

        for (const auto &storage_service: storage_services) {
            if (this->storage_services.find(storage_service->getHostname()) != this->storage_services.end()) {
                this->storage_services[storage_service->getHostname()].push_back(storage_service);
            } else {
                this->storage_services[storage_service->getHostname()] = std::vector<std::shared_ptr<wrench::StorageService>>{storage_service};
            }
        }
        this->storage_selection = std::move(storage_selection);

        if (property_list.find(wrench::CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE) == property_list.end()) {
            // If MAX_ALLOCATION_CHUNK_SIZE was not probided, update it now that we have validated the SSS list
            // (Set as smallest disk capacity in bytes for instance ?)
            // TODO
            // this->setProperty(CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE, "2000000B");
        }

        this->max_chunk_size = this->getPropertyValueAsSizeInByte(CompoundStorageServiceProperty::MAX_ALLOCATION_CHUNK_SIZE);
        this->traceInternalStorageUse(IOAction::None);
    }


    /**
     * @brief Main method of the daemon
     *
     * @return 0 on termination
     */
    int CompoundStorageService::main() {
        //TODO: Use another color?
        TerminalOutput::setThisProcessLoggingColor(TerminalOutput::COLOR_CYAN);
        std::string message = "Compound Storage Service " + this->getName() + "  starting on host " + this->getHostname();
        WRENCH_INFO("%s", message.c_str());

        WRENCH_INFO("CSS - Registered underlying storage services:");
        for (const auto &storage_server: this->storage_services) {
            for (const auto &service: storage_server.second) {
                message = " - " + service->process_name + " on " + service->getHostname();
                WRENCH_INFO("%s", message.c_str());
            }
        }

        /** Main loop **/
        bool comm_ptr_has_been_posted = false;
        simgrid::s4u::CommPtr comm_ptr;
        std::unique_ptr<SimulationMessage> simulation_message;
        while (true) {
            S4U_Simulation::computeZeroFlop();

            // Create an async recv if needed
            if (not comm_ptr_has_been_posted) {
                try {
                    comm_ptr = this->mailbox->get_async<void>((void **) (&(simulation_message)));
                } catch (simgrid::NetworkFailureException &e) {
                    // oh well
                    continue;
                }
                comm_ptr_has_been_posted = true;
            }

            // Create all activities to wait on (only emplace the communicator)
            std::vector<simgrid::s4u::ActivityPtr> pending_activities;
            pending_activities.emplace_back(comm_ptr);

            // Wait one activity (communication in this case) to complete
            int finished_activity_index;
            try {
                finished_activity_index = (int) simgrid::s4u::Activity::wait_any(pending_activities);
            } catch (simgrid::NetworkFailureException &e) {
                comm_ptr_has_been_posted = false;
                continue;
            } catch (std::exception &e) {
                continue;
            }

            // It's a communication
            if (finished_activity_index == 0) {
                comm_ptr_has_been_posted = false;
                if (not processNextMessage(simulation_message.get())) break;
            } else if (finished_activity_index == -1) {
                throw std::runtime_error("wait_any() returned -1. Not sure what to do with this. ");
            }
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
     * @throw std::runtime_error when receiving an unexpected message type.
     * 
     * @return false if the daemon should terminate
     */
    bool CompoundStorageService::processNextMessage(SimulationMessage *message) {
        WRENCH_INFO("CSS::Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message)) {
            return processStopDaemonRequest(msg->ack_mailbox);

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

        WRENCH_INFO("CSS::processStorageSelectionMessage()");

        auto file = msg->file;


        if (this->file_location_mapping.find(file) != this->file_location_mapping.end()) {

            WRENCH_INFO("CSS::lookupOrDesignateStorageService: File %s already known by CSS", file->getID().c_str());

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
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
        if (file_size > this->max_chunk_size) {
            WRENCH_INFO("CSS::lookupOrDesignateStorageService(): Stripping file");
            double remaining = file_size;
            auto part_id = 0;
            while (remaining - this->max_chunk_size > DBL_EPSILON) {
                parts.push_back(
                        this->simulation->addFile(file_name + "_part_" + std::to_string(part_id), this->max_chunk_size));
                // std::make_shared<DataFile>(file_name + "_part_" + std::to_string(part_id), this->max_chunk_size));
                part_id++;
                remaining -= this->max_chunk_size;
            }
            parts.push_back(this->simulation->addFile(file_name + "_part_" + std::to_string(part_id), remaining));
            // parts.push_back(std::make_shared<DataFile>(file_name + "_part_" + std::to_string(part_id), remaining));
        } else {
            parts.push_back(file);
        }

        // Resolve allocations for all parts (possibly only one part)
        std::vector<std::shared_ptr<FileLocation>> designated_locations = {};
        for (const auto &part: parts) {
            WRENCH_INFO("CSS::lookupOrDesignateStorageService(): File %s NOT already known by CSS", part->getID().c_str());
            auto new_loc = this->storage_selection(part, this->storage_services, this->file_location_mapping, designated_locations);
            if (new_loc) {
                new_loc->getStorageService()->reserveSpace(new_loc);
                designated_locations.push_back(new_loc);
            } else {
                WRENCH_INFO("CSS::lookupOrDesignateStorageService(): File %s (or parts) could not be placed on any ss", file->getID().c_str());
                designated_locations = {};
                break;
                // throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(file, this->getSharedPtr<CompoundStorageService>()));
            }
        }

        if (!designated_locations.empty()) {
            this->file_location_mapping[file] = designated_locations;
            WRENCH_INFO("CSS::lookupOrDesignateStorageService(): Local mapping updated");
        }

        S4U_Mailbox::dputMessage(
                msg->answer_mailbox,
                new CompoundStorageAllocationAnswerMessage(
                        designated_locations,
                        this->getMessagePayloadValue(
                                CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        WRENCH_INFO("CSS::lookupOrDesignateStorageService(): Answer sent");

        return true;
    }

    bool CompoundStorageService::processStorageLookupMessage(const CompoundStorageLookupRequestMessage *msg) {

        WRENCH_INFO("CSS::processStorageLookupMessage()");

        auto file = msg->file;

        WRENCH_INFO("CSS::lookupFileLocation(): For file %s", file->getID().c_str());

        if (this->file_location_mapping.find(file) == this->file_location_mapping.end()) {
            WRENCH_INFO("CSS::lookupFileLocation(): File %s is not known by this CompoundStorageService", file->getID().c_str());

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
                    new CompoundStorageLookupAnswerMessage(
                            std::vector<std::shared_ptr<FileLocation>>(),
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        } else {
            auto mapped_locations = this->file_location_mapping[file];
            for (const auto &loc: mapped_locations) {
                WRENCH_INFO("CSS::lookupFileLocation(): File %s is known by this CompoundStorageService and associated to storage service %s",
                            loc->getFile()->getID().c_str(),
                            loc->getStorageService()->getName().c_str());
            }

            S4U_Mailbox::dputMessage(
                    msg->answer_mailbox,
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
     * @param answer_mailbox: the answer mailbox
     * 
     * @return A vector of shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or empty vector if it's not.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupFileLocation(const std::shared_ptr<DataFile> &file, simgrid::s4u::Mailbox *answer_mailbox) {

        WRENCH_INFO("CSS::lookupFileLocation() - DataFile + Mailbox");

        S4U_Mailbox::putMessage(this->mailbox,
                                new CompoundStorageLookupRequestMessage(
                                        answer_mailbox,
                                        file,
                                        this->getMessagePayloadValue(
                                                CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        auto msg = S4U_Mailbox::getMessage<CompoundStorageLookupAnswerMessage>(answer_mailbox, this->network_timeout, "CSS::lookupFileLocation(): Received a totally");

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

        auto temp_mailbox = S4U_Mailbox::getTemporaryMailbox();

        auto locations = this->lookupFileLocation(location->getFile(), temp_mailbox);

        S4U_Mailbox::retireTemporaryMailbox(temp_mailbox);

        return locations;
    }


    /**
     *  @brief Lookup for a DataFile in the internal file mapping of the CompoundStorageService, and if it is not found, 
     *         try to allocate the file on one of the underlying storage services, using the user-provided 'storage_selection'
     *         callback.
     * 
     *  @param file the file of interest
     *
     *  @return A vector of shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or could be allocated
     *          or empty vector if it's not / could not be allocated.
     */
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<DataFile> file, simgrid::s4u::Mailbox *answer_mailbox) {

        WRENCH_INFO("CSS::lookupOrDesignateStorageService() - DataFile + mailbox");

        S4U_Mailbox::putMessage(this->mailbox,
                                new CompoundStorageAllocationRequestMessage(
                                        answer_mailbox,
                                        file,
                                        this->getMessagePayloadValue(
                                                CompoundStorageServiceMessagePayload::STORAGE_SELECTION_PAYLOAD)));

        auto msg = S4U_Mailbox::getMessage<CompoundStorageAllocationAnswerMessage>(answer_mailbox, this->network_timeout, "StorageService::writeFile(): Received a totally");

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
    std::vector<std::shared_ptr<FileLocation>> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<FileLocation> location) {

        WRENCH_INFO("CSS::lookupOrDesignateStorageService() - FileLocation");

        auto temp_mailbox = S4U_Mailbox::getTemporaryMailbox();

        auto locations = this->lookupOrDesignateStorageService(location->getFile(), temp_mailbox);

        S4U_Mailbox::retireTemporaryMailbox(temp_mailbox);

        return locations;
    }


    /**
     * @brief Delete a file on the storage service
     *
     * @param answer_mailbox: the answer mailbox to which the reply from the server should be sent
     * @param location: the location to delete
     * @param wait_for_answer: whether this call should
     */
    void CompoundStorageService::deleteFile(simgrid::s4u::Mailbox *answer_mailbox,
                                            const std::shared_ptr<FileLocation> &location,
                                            bool wait_for_answer) {
        WRENCH_INFO("CSS::deleteFile(): Starting for file %s", location->getFile()->getID().c_str());

        if (!answer_mailbox or !location) {
            throw std::invalid_argument("CSS::deleteFile(): Invalid nullptr arguments");
        }

        if (location->isScratch()) {
            throw std::invalid_argument("CSS::deleteFile(): Cannot be called on a SCRATCH location");
        }


        auto designated_locations = this->lookupFileLocation(location);
        if (designated_locations.empty()) {
            throw ExecutionException(std::make_shared<FileNotFound>(location));
        }

        this->traceInternalStorageUse(IOAction::DeleteStart, designated_locations);

        // Send a message to the storage service's daemon
        for (const auto &loc: designated_locations) {

            WRENCH_INFO("CSS:deleteFile Issuing delete message to SSS %s", loc->getStorageService()->getName().c_str());

            assertServiceIsUp(loc->getStorageService());

            S4U_Mailbox::putMessage(loc->getStorageService()->mailbox,
                                    new StorageServiceFileDeleteRequestMessage(
                                            answer_mailbox,
                                            loc,
                                            this->getMessagePayloadValue(StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

            if (wait_for_answer) {

                // Wait for a reply
                std::unique_ptr<SimulationMessage> message = nullptr;

                auto msg = S4U_Mailbox::getMessage<StorageServiceFileDeleteAnswerMessage>(answer_mailbox, this->network_timeout, "StorageService::deleteFile():");
                // On failure, throw an exception
                if (!msg->success) {
                    throw ExecutionException(std::move(msg->failure_cause));
                }
            }
        }

        // Clean up local map
        this->file_location_mapping.erase(location->getFile());

        // Collect traces
        wrench::AllocationTrace trace;
        // trace.file_name = location->getFile()->getID();
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = designated_locations;
        trace.act = IOAction::DeleteEnd;
        this->delete_traces[location->getFile()->getID()] = trace;

        this->traceInternalStorageUse(IOAction::DeleteEnd, designated_locations);

        WRENCH_INFO("CSS::deleteFile Done for file %s", location->getFile()->getID().c_str());
    }

    /**
     * @brief Asks the storage service whether it holds a file
     *
     * @param answer_mailbox: the answer mailbox to which the reply from the server should be sent
     * @param location: the location to lookup
     *
     * @return true if the file is present, false otherwise
     */
    bool CompoundStorageService::lookupFile(simgrid::s4u::Mailbox *answer_mailbox,
                                            const std::shared_ptr<FileLocation> &location) {
        WRENCH_INFO("CSS::lookupFile(): Lookup for file %s", location->getFile()->getID().c_str());

        if (!answer_mailbox or !location) {
            throw std::invalid_argument("CSS::lookupFile(): Invalid nullptr arguments");
        }

        auto file_parts = this->lookupFileLocation(location);
        if (file_parts.empty()) {
            WRENCH_WARN("File lookup failed because CSS doesn't know of file");
            return false;
        }

        bool available = true;

        // Send a message to the storage service's daemon
        for (const auto &loc: file_parts) {

            assertServiceIsUp(loc->getStorageService());

            S4U_Mailbox::putMessage(loc->getStorageService()->mailbox,
                                    new StorageServiceFileLookupRequestMessage(
                                            answer_mailbox,
                                            location,
                                            this->getMessagePayloadValue(
                                                    StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

            // Wait for a reply
            auto msg = S4U_Mailbox::getMessage<StorageServiceFileLookupAnswerMessage>(answer_mailbox, this->network_timeout, "StorageService::lookupFile():");
            available &= msg->file_is_available;
        }

        return available;
    }

    void CompoundStorageService::copyFile(const std::shared_ptr<FileLocation> &src_location,
                                          const std::shared_ptr<FileLocation> &dst_location) {

        WRENCH_INFO("CSS::copyFile(): Copy %s between %s and %s",
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
     */
    void CompoundStorageService::copyFileIamSource(const std::shared_ptr<FileLocation> &src_location,
                                                   const std::shared_ptr<FileLocation> &dst_location) {
        WRENCH_INFO("CSS::copyFileIamSource()");

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
                        FileLocation::LOCATION(dst_location->getStorageService(), dst_location->getPath(), src_part->getFile()));
            }
        } else {
            dst_parts.push_back(dst_location);// no stripping, dst location doesn't have to change
        }

        // Now run the copy(ies) between the source(s) and the destination(s)
        auto copy_idx = 0;
        int total_parts = src_parts.size();// = dst_parts.size()

        std::vector<simgrid::s4u::Mailbox *> tmp_mailboxes = {};

        while (copy_idx < total_parts) {
            WRENCH_INFO("CSS::copyFileIamSource(): Running StorageService::copyFile for part %i", copy_idx);

            auto tmp_mailbox = S4U_Mailbox::getTemporaryMailbox();
            tmp_mailboxes.push_back(tmp_mailbox);

            assertServiceIsUp(src_parts[copy_idx]->getStorageService());
            assertServiceIsUp(dst_parts[copy_idx]->getStorageService());

            auto file = src_parts[copy_idx]->getFile();
            bool src_is_bufferized = src_parts[copy_idx]->getStorageService()->isBufferized();
            bool src_is_non_bufferized = not src_is_bufferized;
            bool dst_is_bufferized = dst_parts[copy_idx]->getStorageService()->isBufferized();
            bool dst_is_non_bufferized = not dst_is_bufferized;

            simgrid::s4u::Mailbox *mailbox_to_contact;
            if (dst_is_non_bufferized) {
                mailbox_to_contact = dst_parts[copy_idx]->getStorageService()->mailbox;
            } else if (src_is_non_bufferized) {
                mailbox_to_contact = src_parts[copy_idx]->getStorageService()->mailbox;
            } else {
                mailbox_to_contact = dst_parts[copy_idx]->getStorageService()->mailbox;
            }

            this->simulation->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                    src_parts[copy_idx],
                                                                    dst_parts[copy_idx]);
            S4U_Mailbox::dputMessage(
                    mailbox_to_contact,
                    new StorageServiceFileCopyRequestMessage(
                            tmp_mailbox,
                            src_parts[copy_idx],
                            dst_parts[copy_idx],
                            dst_parts[copy_idx]->getStorageService()->getMessagePayloadValue(
                                    StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

            //StorageService::copyFile(src_parts[copy_idx], dst_parts[copy_idx]);
            copy_idx++;
        }

        WRENCH_INFO("CSS::copyFileIamSource(): Copy/ies started");

        // Wait for all replies
        for (const auto &mailbox: tmp_mailboxes) {

            auto msg = S4U_Mailbox::getMessage<StorageServiceFileCopyAnswerMessage>(mailbox);

            if (msg->failure_cause) {
                WRENCH_DEBUG("%s", msg->failure_cause->toString().c_str());
                throw ExecutionException(std::move(msg->failure_cause));
            }

            S4U_Mailbox::retireTemporaryMailbox(mailbox);
        }

        WRENCH_INFO("CSS::copyFileIamSource(): Copy/ies started (answers received)");

        // Cleanup
        if (total_parts > 1) {
            // once all the copies are made, we need to delete parts on destination and merge into a single file
            for (const auto &part: dst_parts) {
                dst_location->getStorageService()->deleteFileAtLocation(part);
            }
            dst_location->getStorageService()->createFileAtLocation(FileLocation::LOCATION(
                    dst_location->getStorageService(), dst_location->getPath(), dst_location->getFile()));
        }

        // Collect traces
        wrench::AllocationTrace trace;
        // trace.file_name = src_location->getFile()->getID();
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = src_parts;
        trace.act = IOAction::CopyToEnd;
        this->copy_traces[src_location->getFile()->getID()] = trace;

        WRENCH_INFO("CSS::copyFileIamSource(): Done (cleanup + tracing)");
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

        WRENCH_INFO("CSS::copyFileIamDestination()");

        // this->traceInternalStorageUse(IOAction::CopyToStart);

        std::vector<std::shared_ptr<FileLocation>> src_parts = {};
        std::vector<std::shared_ptr<FileLocation>> dst_parts = {};

        // Find one or many SSS location(s) for the destination file (depending on whether the original file needs stripping or not)
        dst_parts = this->lookupOrDesignateStorageService(dst_location);
        if (dst_parts.empty()) {
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(dst_location->getFile(), this->getSharedPtr<CompoundStorageService>()));
        }

        this->traceInternalStorageUse(IOAction::CopyToStart, dst_parts);
        WRENCH_INFO("CSS::copyFileIamDestination(): Destination file will be written as %zu file part(s)", dst_parts.size());
        if (dst_parts.size() > 1) {
            for (const auto &dst_part: dst_parts) {
                auto part_size = dst_part->getFile()->getSize();
                dst_part->getFile()->setSize(0);     // make our datafile act as a reference / link
                StorageService::createFileAtLocation(// create link on source storage service
                        FileLocation::LOCATION(
                                src_location->getStorageService(),
                                src_location->getPath(),
                                dst_part->getFile()));
                // Prepare for copy once again
                dst_part->getFile()->setSize(part_size);
                src_parts.push_back(
                        FileLocation::LOCATION(src_location->getStorageService(), src_location->getPath(), dst_part->getFile()));
            }
            WRENCH_INFO("CSS::copyFileIamDestination(): %zu parts created from source file", src_parts.size());
        } else {
            src_parts.push_back(src_location);// no stripping, src location doesn't have to change
        }

        // Now run the copy(ies) between the source(s) and the destination(s)
        auto copy_idx = 0;
        int total_parts = src_parts.size();// = dst_parts.size()

        std::vector<simgrid::s4u::Mailbox *> tmp_mailboxes = {};

        while (copy_idx < total_parts) {
            WRENCH_INFO("CSS::copyFileIamDestination(): Running StorageService::copyFile for part %i", copy_idx);
            WRENCH_INFO("CSS::copyFileIamDestination(): Source = %s with size %f at path %s on  %s%s",
                        src_parts[copy_idx]->getFile()->getID().c_str(),
                        src_parts[copy_idx]->getFile()->getSize(),
                        src_parts[copy_idx]->getPath().c_str(),
                        src_parts[copy_idx]->getStorageService()->getHostname().c_str(),
                        src_parts[copy_idx]->getStorageService()->getBaseRootPath().c_str());
            WRENCH_INFO("CSS::copyFileIamDestination(): Dest = %s with size %f at path %s on  %s%s",
                        dst_parts[copy_idx]->getFile()->getID().c_str(),
                        dst_parts[copy_idx]->getFile()->getSize(),
                        dst_parts[copy_idx]->getPath().c_str(),
                        dst_parts[copy_idx]->getStorageService()->getHostname().c_str(),
                        dst_parts[copy_idx]->getStorageService()->getBaseRootPath().c_str());

            // Useful ?
            assertServiceIsUp(src_parts[copy_idx]->getStorageService());
            assertServiceIsUp(dst_parts[copy_idx]->getStorageService());

            auto file = src_parts[copy_idx]->getFile();
            bool src_is_bufferized = src_parts[copy_idx]->getStorageService()->isBufferized();
            bool src_is_non_bufferized = not src_is_bufferized;
            bool dst_is_bufferized = dst_parts[copy_idx]->getStorageService()->isBufferized();
            bool dst_is_non_bufferized = not dst_is_bufferized;

            simgrid::s4u::Mailbox *mailbox_to_contact;
            if (dst_is_non_bufferized) {
                mailbox_to_contact = dst_parts[copy_idx]->getStorageService()->mailbox;
            } else if (src_is_non_bufferized) {
                mailbox_to_contact = src_parts[copy_idx]->getStorageService()->mailbox;
            } else {
                mailbox_to_contact = dst_parts[copy_idx]->getStorageService()->mailbox;
            }

            // Send a message to the daemon of the dst service
            auto tmp_mailbox = S4U_Mailbox::getTemporaryMailbox();
            tmp_mailboxes.push_back(tmp_mailbox);

            this->simulation->getOutput().addTimestampFileCopyStart(Simulation::getCurrentSimulatedDate(), file,
                                                                    src_parts[copy_idx],
                                                                    dst_parts[copy_idx]);

            // That's quite dirty, but : space is reserved during the call to lookupOrDesignateFileLocation, so that
            // we keep track of future space usage on various storage nodes while allocating multiple chunks of a given
            // file. So right before we actually start the copy, we unreserve space.
            dst_parts[copy_idx]->getStorageService()->unreserveSpace(dst_parts[copy_idx]);
            // TODO : Replace with iputMessage()
            S4U_Mailbox::dputMessage(
                    mailbox_to_contact,
                    new StorageServiceFileCopyRequestMessage(
                            tmp_mailbox,
                            src_parts[copy_idx],
                            dst_parts[copy_idx],
                            dst_parts[copy_idx]->getStorageService()->getMessagePayloadValue(
                                    StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

            copy_idx++;
        }

        WRENCH_INFO("CSS::copyFileIamDestination(): Copy/ies started");

        // Wait for all replies
        for (const auto &mailbox: tmp_mailboxes) {

            auto msg = S4U_Mailbox::getMessage<StorageServiceFileCopyAnswerMessage>(mailbox);

            if (msg->failure_cause) {
                WRENCH_DEBUG("%s", msg->failure_cause->toString().c_str());
                throw ExecutionException(std::move(msg->failure_cause));
            }

            S4U_Mailbox::retireTemporaryMailbox(mailbox);
        }

        WRENCH_INFO("CSS::copyFileIamDestination(): Copy/ies started (answers received)");

        // Once copy is done, remove links (only if source file was stripped)
        if (total_parts > 1) {
            for (const auto &src_part: src_parts) {
                WRENCH_INFO("CSS::copyFileIamDestination(): ");
                auto part_size = src_part->getFile()->getSize();
                src_part->getFile()->setSize(0);// make our datafile a link once again
                WRENCH_INFO("CSS::copyFileIamDestination(): Deleting part %s", src_part->getFile()->getID().c_str());
                StorageService::deleteFileAtLocation(src_part);
                src_part->getFile()->setSize(part_size);
            }
        }

        WRENCH_INFO("CSS::copyFileIamDestination(): Source parts deleted (if needed)");

        // Collect traces
        wrench::AllocationTrace trace;
        // trace.file_name = dst_location->getFile()->getID();
        trace.act = IOAction::WriteEnd;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = dst_parts;
        this->copy_traces[dst_location->getFile()->getID()] = trace;

        this->traceInternalStorageUse(IOAction::CopyToEnd, dst_parts);

        WRENCH_INFO("CSS::copyFileIamDestination(): Done (cleanup + tracing)");
    }


    /**
     * @brief Synchronously write a file to the storage service
     *
     * @param answer_mailbox: the mailbox on which to expect the answer
     * @param location: the location
     * @param wait_for_answer: whether to wait for the answer
     *
     * @throw ExecutionException
     */
    void CompoundStorageService::writeFile(simgrid::s4u::Mailbox *answer_mailbox,
                                           const std::shared_ptr<FileLocation> &location,
                                           bool wait_for_answer) {

        WRENCH_INFO("CSS::writeFile(): Writing file %s", location->getFile()->getID().c_str());
        //this->traceInternalStorageUse(IOAction::WriteStart);

        if (location == nullptr) {
            throw std::invalid_argument("CSS::writeFile(): Invalid arguments");
        }

        this->assertServiceIsUp();

        // Find the file, or allocate file/parts of file onto known SSS
        auto designated_locations = this->lookupOrDesignateStorageService(location);

        if (designated_locations.empty()) {
            throw ExecutionException(std::make_shared<StorageServiceNotEnoughSpace>(location->getFile(), this->getSharedPtr<CompoundStorageService>()));
        }
        std::vector<std::unique_ptr<wrench::StorageServiceFileWriteAnswerMessage>> messages = {};
        std::vector<std::pair<simgrid::s4u::Mailbox *, std::unique_ptr<wrench::StorageServiceFileWriteAnswerMessage>>> mailbox_msg_pairs = {};

        this->traceInternalStorageUse(IOAction::WriteStart, designated_locations);
        WRENCH_INFO("CSS::writeFile(): Destination file %s will have %zu",
                    location->getFile()->getID().c_str(), designated_locations.size());

        // Contact every SimpleStorageService that we want to use, and request a FileWrite
        auto request_count = 0;
        for (auto &dloc: designated_locations) {
            auto tmp_mailbox = S4U_Mailbox::getTemporaryMailbox();
            mailbox_msg_pairs.push_back(std::make_pair(tmp_mailbox, nullptr));

            WRENCH_INFO("CSS:writeFile(): Sending write request for part %d to %s", request_count, dloc->getStorageService()->getName().c_str());

            // That's quite dirty, but : space is reserved during the call to lookupOrDesignateFileLocation, so that
            // we keep track of future space usage on various storage nodes while allocating multiple chunks of a given
            // file. So right before we actually start the copy, we unreserve space.
            dloc->getStorageService()->unreserveSpace(dloc);
            // TODO : Replace with iputMessage()
            S4U_Mailbox::dputMessage(
                    dloc->getStorageService()->mailbox,
                    new StorageServiceFileWriteRequestMessage(
                            tmp_mailbox,
                            simgrid::s4u::this_actor::get_host(),
                            dloc,
                            this->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));
            request_count++;
        }

        for (auto &mailbox_pair: mailbox_msg_pairs) {
            // Wait for answer to current reqeust
            auto msg = S4U_Mailbox::getMessage<StorageServiceFileWriteAnswerMessage>(mailbox_pair.first, this->network_timeout, "CSS::writeFile(): ");
            if (not msg->success)
                throw ExecutionException(msg->failure_cause);

            mailbox_pair.second = std::move(msg);
        }

        WRENCH_INFO("CSS::writeFile(): All requests sent and validated");

        for (const auto &mailbx_msg: mailbox_msg_pairs) {

            // Update buffer size according to which storage service actually answered.
            auto buffer_size = mailbx_msg.second->buffer_size;

            if (buffer_size >= 1) {

                auto file = location->getFile();
                for (auto const &dwmb: mailbx_msg.second->data_write_mailboxes_and_bytes) {
                    // Bufferized
                    double remaining = dwmb.second;
                    while (remaining - buffer_size > DBL_EPSILON) {
                        S4U_Mailbox::dputMessage(dwmb.first,
                                                 new StorageServiceFileContentChunkMessage(
                                                         file, buffer_size, false));
                        remaining -= buffer_size;
                    }
                    S4U_Mailbox::dputMessage(dwmb.first, new StorageServiceFileContentChunkMessage(
                                                                 file, remaining, true));
                }
            }
        }

        for (const auto &mailbx_msg: mailbox_msg_pairs) {
            WRENCH_INFO("CSS::writeFile(): Waiting for final ack");
            S4U_Mailbox::getMessage<StorageServiceAckMessage>(mailbx_msg.first, "CSS::writeFile(): ");
            S4U_Mailbox::retireTemporaryMailbox(mailbx_msg.first);
        }

        WRENCH_INFO("CSS::writeFile(): All writes done and ack");
        this->traceInternalStorageUse(IOAction::WriteEnd, designated_locations);

        // Collect traces
        wrench::AllocationTrace trace;
        trace.act = IOAction::WriteEnd;
        trace.ts = S4U_Simulation::getClock();
        trace.internal_locations = designated_locations;
        this->write_traces[location->getFile()->getID()] = trace;
    }

    /**
     * @brief Read a file from the storage service
     *
     * @param answer_mailbox: the mailbox on which to expect the answer
     * @param location: the location
     * @param num_bytes: the number of bytes to read
     * @param wait_for_answer: whether to wait for the answer
     */
    void CompoundStorageService::readFile(simgrid::s4u::Mailbox *answer_mailbox,
                                          const std::shared_ptr<FileLocation> &location,
                                          double num_bytes,
                                          bool wait_for_answer) {
        WRENCH_INFO("CSS::readFile(): Reading file %s", location->getFile()->getID().c_str());

        if (!answer_mailbox or !location or (num_bytes < 0.0)) {
            throw std::invalid_argument("StorageService::readFile(): Invalid nullptr/0 arguments");
        }

        assertServiceIsUp(this->shared_from_this());

        auto designated_locations = this->lookupFileLocation(location);
        if (designated_locations.empty()) {
            throw ExecutionException(std::make_shared<FileNotFound>(location));
        }

        // Contact every SSS
        auto left_to_receive = designated_locations.size();
        std::vector<std::pair<simgrid::s4u::Mailbox *, std::unique_ptr<wrench::StorageServiceFileReadAnswerMessage>>> mailbox_msg_pairs = {};

        for (const auto &dloc: designated_locations) {
            WRENCH_INFO("CSS::readFile(): Sending read file request for file %s at path %s, to storage service %s",
                        dloc->getFile()->getID().c_str(),
                        dloc->getPath().c_str(),
                        dloc->getStorageService()->getName().c_str());

            auto tmp_mailbox = S4U_Mailbox::getTemporaryMailbox();
            mailbox_msg_pairs.push_back(std::make_pair(tmp_mailbox, nullptr));

            // REPLACE WITH iputMessage()?
            S4U_Mailbox::dputMessage(
                    dloc->getStorageService()->mailbox,
                    new StorageServiceFileReadRequestMessage(
                            tmp_mailbox,
                            simgrid::s4u::this_actor::get_host(),
                            dloc,
                            dloc->getFile()->getSize(),
                            dloc->getStorageService()->getMessagePayloadValue(
                                    CompoundStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));
        }

        for (auto &mailbox_pair: mailbox_msg_pairs) {
            // Wait for answer to current reqeust
            auto msg = S4U_Mailbox::getMessage<StorageServiceFileReadAnswerMessage>(mailbox_pair.first, this->network_timeout, "CSS::readFile(): ");
            if (not msg->success)
                throw ExecutionException(msg->failure_cause);

            mailbox_pair.second = std::move(msg);
        }

        WRENCH_INFO("CSS::readFile(): All requests sent and validated");

        for (const auto &mailbx_msg: mailbox_msg_pairs) {

            if (mailbx_msg.second->buffer_size < 1) {
                // Non-Bufferized
                // Just wait for the final ack (no timeout!)
                S4U_Mailbox::getMessage<StorageServiceAckMessage>(mailbx_msg.first, "CSS::readFile(): ");
            } else {
                unsigned long number_of_sources = mailbx_msg.second->number_of_sources;

                // Otherwise, retrieve the file chunks until the last one is received
                // Noting that we have multiple sources
                unsigned long num_final_chunks_received = 0;
                while (true) {
                    std::shared_ptr<StorageServiceFileContentChunkMessage> file_content_chunk_msg = nullptr;
                    try {
                        file_content_chunk_msg = S4U_Mailbox::getMessage<StorageServiceFileContentChunkMessage>(mailbx_msg.second->mailbox_to_receive_the_file_content, "StorageService::readFile(): Received an");
                    } catch (...) {
                        S4U_Mailbox::retireTemporaryMailbox(mailbx_msg.second->mailbox_to_receive_the_file_content);
                        throw;
                    }
                    if (file_content_chunk_msg->last_chunk) {
                        num_final_chunks_received++;
                        if (num_final_chunks_received == mailbx_msg.second->number_of_sources) {
                            break;
                        }
                    }
                }

                S4U_Mailbox::retireTemporaryMailbox(mailbx_msg.second->mailbox_to_receive_the_file_content);

                //Waiting for all the final acks
                for (unsigned long source = 0; source < number_of_sources; source++) {
                    S4U_Mailbox::getMessage<StorageServiceAckMessage>(mailbx_msg.first, this->network_timeout, "StorageService::readFile(): Received an");
                }
            }

            S4U_Mailbox::retireTemporaryMailbox(mailbx_msg.first);
        }

        WRENCH_INFO("CSS::readFile(): All read done and ack");

        // Collect traces
        wrench::AllocationTrace trace;
        // trace.file_name = location->getFile()->getID();
        trace.ts = S4U_Simulation::getClock();
        trace.act = IOAction::ReadEnd;
        trace.internal_locations = designated_locations;
        // this->read_traces[location->getFile()->getID()] = trace;
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
     * @param path: the path
     *
     * @return A number of bytes
     */
    double CompoundStorageService::getTotalSpace() {
        //        WRENCH_INFO("CompoundStorageService::getTotalSpace");
        double free_space = 0.0;
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
     * @throw ExecutionException
     *
     */
    double CompoundStorageService::getTotalFreeSpaceAtPath(const std::string &path) {
        WRENCH_INFO("CSS::getFreeSpace Forwarding request to internal services");

        double free_space = 0.0;
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
     *  @throw std::logic_error
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
        WRENCH_INFO("CSS::getAllServices");
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
            if (designated_storage_service)
                return designated_storage_service->getFileLastWriteDate(location);
        }

        return 0.0;// FIX THAT.
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
     * @param ack_mailbox: the mailbox to which the ack should be sent
     * 
     * @throw wrench::ExecutionException if communication fails.
     * 
     * @return false if the daemon should terminate
     */
    bool CompoundStorageService::processStopDaemonRequest(simgrid::s4u::Mailbox *ack_mailbox) {
        try {
            S4U_Mailbox::putMessage(ack_mailbox,
                                    new ServiceDaemonStoppedMessage(this->getMessagePayloadValue(
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

        if (locations.empty()) {

            for (const auto &storage: this->storage_services) {
                for (const auto &storage_service: storage.second) {
                    DiskUsage disk_usage;
                    disk_usage.service = storage_service;
                    disk_usage.load = storage_service->getLoad();// number of concurrent reads
                    disk_usage.free_space = storage_service->traceTotalFreeSpace();
                    trace.disk_usage.push_back(disk_usage);
                }
            }

        } else {

            for (const auto &location: locations) {
                auto storage_service = location->getStorageService();
                DiskUsage disk_usage;
                disk_usage.service = storage_service;
                disk_usage.file_name = location->getFile()->getID();
                disk_usage.load = storage_service->getLoad();// number of concurrent reads
                disk_usage.free_space = storage_service->traceTotalFreeSpace();
                trace.disk_usage.push_back(disk_usage);
            }
        }

        this->internal_storage_use.push_back(std::make_pair(ts, trace));
    }

};// namespace wrench

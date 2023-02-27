#include <wrench/services/storage/compound/CompoundStorageService.h>
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
     * 
     *  @return nullptr (instead of a valid FileLocation)
    */
    std::shared_ptr<FileLocation> nullptrStorageServiceSelection(
            const std::shared_ptr<DataFile> &file,
            const std::set<std::shared_ptr<StorageService>> &resources,
            const std::map<std::shared_ptr<DataFile>, std::shared_ptr<FileLocation>> &mapping) {
        return nullptr;
    }


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

        /* // This should eventually be allowed, currently trying to fix it.
            if (std::any_of(storage_services.begin(), storage_services.end(), [](const auto& elem){ return elem->isBufferized(); })) {
                throw std::invalid_argument("CompoundStorageService can't deal with bufferized StorageServices");
            }
            */

        // CSS should be non-bufferized, as it actually doesn't copy / transfer anything
        // and this allows it to receive message requests for copy (otherwise, src storage service might receive it)
        this->storage_services = storage_services;
        this->storage_selection = std::move(storage_selection);
        this->isStorageSelectionUserProvided = storage_selection_user_provided;

        // Dummy logical file system
        this->file_systems[LogicalFileSystem::DEV_NULL] = LogicalFileSystem::createLogicalFileSystem(
                this->getHostname(),
                this,
                LogicalFileSystem::DEV_NULL,
                this->getPropertyValueAsString(wrench::StorageServiceProperty::CACHING_BEHAVIOR));
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

        WRENCH_INFO("Registered underlying storage services:");
        for (const auto &ss: this->storage_services) {
            message = " - " + ss->process_name + " on " + ss->getHostname();
            WRENCH_INFO("%s", message.c_str());
            //            for (const auto &mnt: ss->getMountPoints()) {
            //                WRENCH_INFO("  - %s", mnt.c_str());
            //            }
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
        WRENCH_INFO("Got a [%s] message", message->getName().c_str());

        if (auto msg = dynamic_cast<ServiceStopDaemonMessage *>(message)) {
            return processStopDaemonRequest(msg->ack_mailbox);

        } else if (auto msg = dynamic_cast<StorageServiceFileDeleteRequestMessage *>(message)) {
            return processFileDeleteRequest(msg);

        } else if (auto msg = dynamic_cast<StorageServiceFileLookupRequestMessage *>(message)) {
            return processFileLookupRequest(msg);

        } else if (auto msg = dynamic_cast<StorageServiceFileWriteRequestMessage *>(message)) {
            return processFileWriteRequest(msg);

        } else if (auto msg = dynamic_cast<StorageServiceFileReadRequestMessage *>(message)) {
            return processFileReadRequest(msg);

        } else if (auto msg = dynamic_cast<StorageServiceFileCopyRequestMessage *>(message)) {
            return processFileCopyRequest(msg);

        } else {
            throw std::runtime_error(
                    "CompoundStorageService::processNextMessage(): Unexpected [" + message->getName() + "] message." +
                    "This is only an abstraction layer and it can't be used as an actual storage service");
        }
    }

    /**
     * @brief Lookup for a DataFile in the internal file mapping of the CompoundStorageService (a simplified FileRegistry)
     *
     * @param file: the file of interest
     * 
     * @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or nullptr if it's not.
     */
    std::shared_ptr<FileLocation> CompoundStorageService::lookupFileLocation(const std::shared_ptr<DataFile> &file) {
        WRENCH_DEBUG("lookupFileLocation: For file %s", file->getID().c_str());

        if (this->file_location_mapping.find(file) == this->file_location_mapping.end()) {
            WRENCH_DEBUG("lookupFileLocation: File %s is not known by this CompoundStorageService", file->getID().c_str());
            return nullptr;
        } else {
            auto mapped_location = this->file_location_mapping[file];
            WRENCH_DEBUG("lookupFileLocation: File %s is known by this CompoundStorageService and associated to storage service %s",
                         mapped_location->getFile()->getID().c_str(),
                         mapped_location->getStorageService()->getName().c_str());
            return mapped_location;
        }
    }

    /** 
     *  @brief Lookup for a FileLocation (using its internal DataFile) in the internal file mapping of the CompoundStorageService 
     *         (a simplified FileRegistry) 
     * 
     *  @param location: the location of interest
     *
     *  @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or nullptr if it's not.
     */
    std::shared_ptr<FileLocation> CompoundStorageService::lookupFileLocation(const std::shared_ptr<FileLocation> &location) {
        return this->lookupFileLocation(location->getFile());
    }


    /**
     *  @brief Lookup for a DataFile in the internal file mapping of the CompoundStorageService, and if it is not found, 
     *         try to allocate the file on one of the underlying storage services, using the user-provided 'storage_selection'
     *         callback.
     * 
     *  @param concrete_file_location: the file of interest
     *
     *  @return A shared_ptr on a FileLocation if the DataFile is known to the CompoundStorageService or could be allocated
     *          or nullptr if it's not.
     */
    std::shared_ptr<FileLocation> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<DataFile> concrete_file_location) {
        if (this->lookupFileLocation(concrete_file_location)) {
            WRENCH_DEBUG("lookupOrDesignateStorageService: File %s already known by CSS", concrete_file_location->getID().c_str());
            return this->file_location_mapping[concrete_file_location];
        }

        WRENCH_DEBUG("lookupOrDesignateStorageService: File %s NOT already known by CSS", concrete_file_location->getID().c_str());
        auto designatedLocation = this->storage_selection(concrete_file_location, this->storage_services, this->file_location_mapping);

        if (!designatedLocation) {
            WRENCH_DEBUG("lookupOrDesignateStorageService: File %s could not be placed on any ss", concrete_file_location->getID().c_str());
        } else {
            WRENCH_DEBUG("lookupOrDesignateStorageService: Registering file %s on storage service %s, at path %s",
                         designatedLocation->getFile()->getID().c_str(),
                         designatedLocation->getStorageService()->getName().c_str(),
                         designatedLocation->getPath().c_str());

            // Supposing (and it better be true) that DataFiles are unique throught a given simulation run, even among various jobs.
            this->file_location_mapping[designatedLocation->getFile()] = designatedLocation;
        }

        return designatedLocation;
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
    std::shared_ptr<FileLocation> CompoundStorageService::lookupOrDesignateStorageService(const std::shared_ptr<FileLocation> location) {
        return this->lookupOrDesignateStorageService(location->getFile());
    }

    /**
     * @brief Handle (and intercept) a file delete request
     *
     * @param msg: The StorageServiceFileDeleteRequestMessage received by a CompoundStorageService
     *
     * @return true if this process should keep running
     */
    bool CompoundStorageService::processFileDeleteRequest(StorageServiceFileDeleteRequestMessage *msg) {
        auto designated_location = this->lookupFileLocation(msg->location);
        if (!designated_location) {
            WRENCH_WARN("processFileDeleteRequest: Unable to find file %s",
                        msg->location->getFile()->getID().c_str());
            try {
                S4U_Mailbox::dputMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileDeleteAnswerMessage(
                                nullptr,
                                this->getSharedPtr<CompoundStorageService>(),
                                false,
                                std::shared_ptr<FailureCause>(new FileNotFound(msg->location)),
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_DELETE_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {}

            return true;
        }

        S4U_Mailbox::putMessage(
                designated_location->getStorageService()->mailbox,
                new StorageServiceFileDeleteRequestMessage(
                        msg->answer_mailbox,
                        designated_location,
                        designated_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_DELETE_REQUEST_MESSAGE_PAYLOAD)));

        return true;
    }

    /**
     * @brief Handle (and intercept) a file lookup request
     *
     * @param msg: The StorageServiceFileLookupRequestMessage received by a CompoundStorageService
     *
     * @return true if this process should keep running
     */
    bool CompoundStorageService::processFileLookupRequest(StorageServiceFileLookupRequestMessage *msg) {
        auto designated_location = this->lookupFileLocation(msg->location);
        if (!designated_location) {
            WRENCH_WARN("processFileLookupRequest: Unable to find file %s", msg->location->getFile()->getID().c_str());
            // Abort because we don't know the file (it should have been written or copied somewhere before the lookup happens)
            try {
                S4U_Mailbox::dputMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileLookupAnswerMessage(
                                nullptr, false,
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_LOOKUP_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {}

            return true;
        }

        // The file is known, we can forward the request to the underlying designated StorageService
        S4U_Mailbox::putMessage(
                designated_location->getStorageService()->mailbox,
                new StorageServiceFileLookupRequestMessage(
                        msg->answer_mailbox,
                        FileLocation::LOCATION(
                                designated_location->getStorageService(),
                                designated_location->getPath(),
                                designated_location->getFile()),
                        designated_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD)));

        return true;
    }

    /**
     * @brief Handle (and intercept) a file copy request
     *
     * @param msg: The StorageServiceFileCopyRequestMessage received by a CompoundStorageService
     *
     * @return true if this process should keep running
     */
    bool CompoundStorageService::processFileCopyRequest(StorageServiceFileCopyRequestMessage *msg) {

        // If source location references a CSS, it must already be known to the CSS
        auto final_src = msg->src;
        if (std::dynamic_pointer_cast<CompoundStorageService>(msg->src->getStorageService())) {
            final_src = this->lookupFileLocation(msg->src->getFile());
        }
        // If destination location references a CSS, it must already exist OR we must be able to allocate it
        auto final_dst = msg->dst;
        if (std::dynamic_pointer_cast<CompoundStorageService>(msg->dst->getStorageService())) {
            final_dst = this->lookupOrDesignateStorageService(msg->dst);
        }

        //        std::cerr << "FINAL DST = " << (final_dst == nullptr) << "\n";

        // Error case - src
        if (!final_src) {
            WRENCH_WARN("processFileCopyRequest: Source %s is a CompoundStorageService and file doesn't exist yet.",
                        msg->src->getStorageService()->getName().c_str());
            try {
                std::string error = "CompoundStorageService can't be the source of a file copy if the file has"
                                    " not already been written or copied to it.";

                S4U_Mailbox::putMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                msg->src,
                                msg->dst,
                                false,
                                std::shared_ptr<FailureCause>(new NotAllowed(
                                        this->getSharedPtr<CompoundStorageService>(),
                                        error)),
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {}

            return true;
        }

        // Error case - dst
        if (!final_dst) {
            WRENCH_WARN("processFileCopyRequest: Destination file %s not found or not enough space left",
                        msg->dst->getFile()->getID().c_str());

            std::shared_ptr<FailureCause> failure_cause;
            if (!this->isStorageSelectionUserProvided) {
                std::string err = "CompoundStorageService doesn't know dst file and can't allocate it because no storage_selection callback was provided";
                failure_cause = std::make_shared<NotAllowed>(
                        this->getSharedPtr<CompoundStorageService>(),
                        err);
            } else {
                failure_cause = std::make_shared<StorageServiceNotEnoughSpace>(
                        msg->dst->getFile(),
                        this->getSharedPtr<CompoundStorageService>());
            }

            try {
                S4U_Mailbox::putMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileCopyAnswerMessage(
                                msg->src,
                                msg->dst,
                                false,
                                failure_cause,
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_COPY_ANSWER_MESSAGE_PAYLOAD)));
            } catch (ExecutionException &e) {}

            return true;
        }

        // Depending on whether we updated source or destination, we now need to find
        // to which to forward the message
        bool src_is_bufferized = final_src->getStorageService()->isBufferized();
        bool dst_is_bufferized = final_dst->getStorageService()->isBufferized();

        simgrid::s4u::Mailbox *mailbox_to_contact;
        if (!dst_is_bufferized) {
            mailbox_to_contact = final_dst->getStorageService()->mailbox;
        } else if (!src_is_bufferized) {
            mailbox_to_contact = final_src->getStorageService()->mailbox;
        } else {
            mailbox_to_contact = final_dst->getStorageService()->mailbox;
        }

        S4U_Mailbox::putMessage(
                mailbox_to_contact,
                new StorageServiceFileCopyRequestMessage(
                        msg->answer_mailbox,
                        final_src,
                        final_dst,
                        final_dst->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_COPY_REQUEST_MESSAGE_PAYLOAD)));

        return true;
    }

    /**
     * @brief Handle (and intercept) a file write request. 
     *        Note: Currently it's not reachable, because we also override a writeFile,
     *        but intercepting the write message would probably be better.
     *
     * @param msg: The StorageServiceFileWriteRequestMessage received by a CompoundStorageService
     * @return true if this process should keep running
     */
    bool CompoundStorageService::processFileWriteRequest(StorageServiceFileWriteRequestMessage *msg) {
        auto designated_location = this->lookupOrDesignateStorageService(msg->location);
        if (!designated_location) {
            WRENCH_WARN("processFileWriteRequest: Destination file %s not found or not enough space left",
                        msg->location->getFile()->getID().c_str());
            try {
                S4U_Mailbox::dputMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileWriteAnswerMessage(
                                msg->location,
                                false,
                                std::shared_ptr<FailureCause>(new FileNotFound(msg->location)),
                                {},
                                0,
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_WRITE_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {}

            return true;
        }

        // The file is known or added to the local mapping, we can forward the request to the underlying designated StorageService
        S4U_Mailbox::putMessage(
                designated_location->getStorageService()->mailbox,
                new StorageServiceFileWriteRequestMessage(
                        msg->answer_mailbox,
                        msg->requesting_host,
                        designated_location,
                        this->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_WRITE_REQUEST_MESSAGE_PAYLOAD)));

        return true;
    }

    /**
     * @brief Handle (and intercept) a file read request. 
     *
     * @param msg: The StorageServiceFileReadRequestMessage received by a CompoundStorageService
     * @return true if this process should keep running
     */
    bool CompoundStorageService::processFileReadRequest(StorageServiceFileReadRequestMessage *msg) {
        auto designated_location = this->lookupFileLocation(msg->location);
        if (!designated_location) {
            WRENCH_WARN("processFileReadRequest: file %s not found", msg->location->getFile()->getID().c_str());
            try {
                S4U_Mailbox::dputMessage(
                        msg->answer_mailbox,
                        new StorageServiceFileReadAnswerMessage(
                                msg->location,
                                false,
                                std::shared_ptr<FailureCause>(new FileNotFound(msg->location)),
                                nullptr,
                                0,
                                1,
                                this->getMessagePayloadValue(
                                        CompoundStorageServiceMessagePayload::FILE_READ_ANSWER_MESSAGE_PAYLOAD)));
            } catch (wrench::ExecutionException &e) {}

            return true;
        }

        WRENCH_DEBUG("processFileReadRequest: Going to read file %s on storage service %s, at path %s",
                     designated_location->getFile()->getID().c_str(),
                     designated_location->getStorageService()->getName().c_str(),
                     designated_location->getPath().c_str());

        S4U_Mailbox::putMessage(
                designated_location->getStorageService()->mailbox,
                new StorageServiceFileReadRequestMessage(
                        msg->answer_mailbox,
                        msg->requesting_host,
                        designated_location,
                        msg->num_bytes_to_read,
                        designated_location->getStorageService()->getMessagePayloadValue(
                                StorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD)));

        return true;
    }


    /**
     * @brief Get the load (number of concurrent reads) on the storage service
     *        Not implemented yet for CompoundStorageService (is it needed?)
     * @return the load on the service (currently throws)
     */
    double CompoundStorageService::getLoad() {
        WRENCH_WARN("CompoundStorageService::getLoad Not implemented");
        throw std::logic_error("CompoundStorageService::getLoad(): Not implemented. "
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
        for (const auto &service: this->storage_services) {
            auto service_name = service->getName();
            free_space += service->getTotalSpace();
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
        WRENCH_DEBUG("CompoundStorageService::getFreeSpace Forwarding request to internal services");

        double free_space = 0.0;
        for (const auto &service: this->storage_services) {
            free_space += service->getTotalFreeSpaceAtPath(path);
        }
        return free_space;
    }

    /** 
     *  @brief setIsScratch can't be used on a CompoundStorageService because it doesn't have any actual storage resources.
     *  @param is_scratch true or false
     *  @throw std::logic_error
     */
    void CompoundStorageService::setIsScratch(bool is_scratch) {
        WRENCH_WARN("CompoundStorageService::setScratch Forbidden because CompoundStorageService doesn't manage any storage resources itself");
        throw std::logic_error("CompoundStorageService can't be setup as a scratch space, it is only an abstraction layer.");
    }

    /**
     * @brief Return the set of all services accessible through this CompoundStorageService
     * 
     * @return The set of known StorageServices.
    */
    std::set<std::shared_ptr<StorageService>> &CompoundStorageService::getAllServices() {
        WRENCH_DEBUG("CompoundStorageService::getAllServices");
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
            throw std::invalid_argument("CompoundStorageService::getFileLastWriteDate(): Invalid nullptr argument");
        }
        if (!this->hasFile(location)) {
            throw std::invalid_argument("CompoundStorageService::getFileLastWriteDate(): File not known to the CompoundStorageService. Unable to forward to underlying StorageService");
        }
        auto file = location->getFile();

        if ((this->file_location_mapping.find(file) == this->file_location_mapping.end()) or
            (this->file_location_mapping[file]->getPath() != FileLocation::sanitizePath(location->getPath()))) {
            return -1;
        }

        auto designated_storage_service = std::dynamic_pointer_cast<SimpleStorageService>(this->file_location_mapping[file]->getStorageService());
        if (designated_storage_service) {
            return designated_storage_service->getFileLastWriteDate(location);
        } else {
            return -1;
        }
    }

    /**
     * @brief Check (outside of simulation time) whether the storage service has a file
     *
     * @param location a location
     *
     * @return true if the file is present, false otherwise
     */
    bool CompoundStorageService::hasFile(const std::shared_ptr<FileLocation> &location) {
        auto file_location = this->lookupFileLocation(location->getFile());
        if (!file_location) {
            WRENCH_DEBUG("hasFile: File %s not found", location->getFile()->getID().c_str());
            return false;
        }
        if (file_location->getPath() != location->getPath()) {
            WRENCH_DEBUG("hasFile: File %s found, but path %s doesn't match internal path %s",
                         location->getFile()->getID().c_str(), location->getPath().c_str(), file_location->getPath().c_str());
            return false;
        }

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

};// namespace wrench

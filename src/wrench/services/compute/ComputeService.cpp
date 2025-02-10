/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/storage/simple/SimpleStorageService.h>
#include <wrench/exceptions/ExecutionException.h>
#include <wrench/logging/TerminalOutput.h>
#include <wrench/services/compute/ComputeService.h>
#include <wrench/services/compute/ComputeServiceProperty.h>
#include <wrench/services/compute/ComputeServiceMessagePayload.h>
#include <wrench/services/compute/ComputeServiceMessage.h>
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>

#include <utility>

WRENCH_LOG_CATEGORY(wrench_core_compute_service, "Log category for Compute Service");

namespace wrench {

    /**
     * @brief Stop the compute service
     */
    void ComputeService::stop() {
        this->stop(false, ComputeService::TerminationCause::TERMINATION_NONE);
    }

    /**
     * @brief Stop the compute service
     * @param send_failure_notifications: whether to send job failure notifications or not
     * @param termination_cause: the cause (reason) of the service's termination
     */
    void ComputeService::stop(bool send_failure_notifications, ComputeService::TerminationCause termination_cause) {
        /** THIS IS CODE DUPLICATION FROM Service::stop(), which is not great **/

        // Do nothing if the service is already down
        if ((this->state == Service::DOWN) or (this->shutting_down)) {
            return;
        }
        this->shutting_down = true;// This is to avoid another process calling stop() and being stuck

        WRENCH_INFO("Telling the daemon listening on (%s) to terminate", this->commport->get_cname());

        // Send a termination message to the daemon's commport - SYNCHRONOUSLY
        auto ack_commport = S4U_Daemon::getRunningActorRecvCommPort();
        try {
            this->commport->dputMessage(new ServiceStopDaemonMessage(
                    ack_commport,
                    send_failure_notifications,
                    termination_cause,
                    this->getMessagePayloadValue(
                            ServiceMessagePayload::STOP_DAEMON_MESSAGE_PAYLOAD)));
        } catch (ExecutionException &e) {
            this->shutting_down = false;
            throw;
        }

        // Wait for the ack
        try {
            ack_commport->getMessage<ServiceDaemonStoppedMessage>(
                    this->network_timeout,
                    "ComputeService::stop(): Received an");
        } catch (...) {
            // If we don't get the ack we assum it's down anyway
        }

        // Set the service state to down
        this->shutting_down = false;
        this->state = Service::DOWN;
    }

    /**
     * @brief Submit a job to the compute service
     * @param job: the job
     * @param service_specific_args: arguments specific to compute services when needed:
     *      - to a BareMetalComputeService: {}
     *          - If no entry is provided for an actionID, the service will pick on which host and with how many cores to run the task
     *          - If a number of cores is provided (e.g., {"action1", "12"}), the service will pick the host on which to run the task
     *          - If a hostname and a number of cores is provided (e.g., {"action1", "host1:12"}, the service will run the action on that host
     *            with the specified number of cores
     *      - to a BatchComputeService: {"-t":"<int>","-N":"<int>","-c":"<int>"[,{"-u":"<string>"}], [{actionID:[host[:num_cores]]}
     *         - "-t": number of requested job duration in seconds
     *         - "-N": number of requested compute hosts
     *         - "-c": number of requested cores per compute host
     *         - "-u": username (optional)
     *      - to a CloudComputeService: {}
     *
     */
    void ComputeService::submitJob(const std::shared_ptr<CompoundJob> &job, const std::map<std::string, std::string> &service_specific_args) {
        if (job == nullptr) {
            throw std::invalid_argument("ComputeService::submitJob(): invalid argument");
        }

        assertServiceIsUp();

        this->submitCompoundJob(job, service_specific_args);
    }

    /**
     * @brief Terminate a previously-submitted job (which may or may not be running yet)
     *
     * @param job: the job to terminate
     *
     */
    void ComputeService::terminateJob(const std::shared_ptr<CompoundJob> &job) {
        if (job == nullptr) {
            throw std::invalid_argument("ComputeService::terminateJob(): invalid argument");
        }

        assertServiceIsUp();

        this->terminateCompoundJob(job);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the compute service runs
     * @param service_name: the name of the compute service
     * @param scratch_space_mount_point: the service's scratch space's mount point ("" if none)
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string &service_name,
                                   const std::string &scratch_space_mount_point) : Service(hostname, service_name) {
        this->state = ComputeService::UP;
        // Check that mount point makes sense
        if ((not scratch_space_mount_point.empty()) and (not Simulation::hostHasMountPoint(hostname, scratch_space_mount_point))) {
            throw std::invalid_argument("ComputeService::ComputeService(): Host " + hostname + " does not have a disk mounted at " + scratch_space_mount_point);
        }
        this->scratch_space_mount_point = scratch_space_mount_point;
    }

    /**
     * @brief Start the compute service's scratch space service
     */
    void ComputeService::startScratchStorageService() {
        if (this->scratch_space_storage_service) return;    // Already started by somebody else
        if (this->scratch_space_mount_point.empty()) return;// No mount point provided


        if (wrench::Simulation::isLinkShutdownSimulationEnabled() and (this->getPropertyValueAsSizeInByte(ComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE) == 0)) {
            throw std::runtime_error("ComputeService::startScratchStorageService(): Compute service " + this->name +
                                     " cannot start a scratch service with (default) buffer size 0 because link shutdown " +
                                     "simulation is enabled. Set a non-zero buffer size by setting the "
                                     "SCRATCH_SPACE_BUFFER_SIZE property of this compute service");
        }

        auto ss = SimpleStorageService::createSimpleStorageService(
                hostname,
                {scratch_space_mount_point},
                {{wrench::SimpleStorageServiceProperty::BUFFER_SIZE, this->getPropertyValueAsString(ComputeServiceProperty::SCRATCH_SPACE_BUFFER_SIZE)}}, {});

        ss->setIsScratch(true);
        // Set the storage service's network timeout to that of this compute service
        ss->setNetworkTimeoutValue(this->getNetworkTimeoutValue());
        this->scratch_space_storage_service =
                this->simulation_->startNewService(ss);
    }

    /**
     * @brief Constructor
     *
     * @param hostname: the name of the host on which the compute service runs
     * @param service_name: the name of the compute service
     * @param scratch_space: scratch storage space of the compute service (nullptr if none)
     */
    ComputeService::ComputeService(const std::string &hostname,
                                   const std::string &service_name,
                                   std::shared_ptr<StorageService> scratch_space) : Service(hostname, service_name) {
        this->state = ComputeService::UP;
        this->scratch_space_storage_service = std::move(scratch_space);
    }

    /**
     * @brief Get the number of hosts that the compute service manages
     * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
     *                     perform this operation in zero simulated time.
     * @return the host count
     *
     */
    unsigned long ComputeService::getNumHosts(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_hosts", simulate_it);

        return static_cast<unsigned long>((*(dict.begin())).second);
    }

    /**
      * @brief Get the list of the compute service's compute host
      * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
      *                     perform this operation in zero simulated time.
      * @return a vector of hostnames
      *
      */
    std::vector<std::string> ComputeService::getHosts(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_cores", simulate_it);

        std::vector<std::string> to_return;
        to_return.reserve(dict.size());

        for (auto const &x: dict) {
            to_return.emplace_back(x.first);
        }

        return to_return;
    }


    /**
      * @brief Get core counts for each of the compute service's host
      * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
      *                     perform this operation in zero simulated time.
      * @return a map of core counts, indexed by hostnames
      *
      */
    std::map<std::string, unsigned long> ComputeService::getPerHostNumCores(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_cores", simulate_it);

        std::map<std::string, unsigned long> to_return;

        for (auto const &x: dict) {
            to_return.insert(std::make_pair(x.first, static_cast<unsigned long>(x.second)));
        }

        return to_return;
    }

    /**
      * @brief Get the total core counts for all hosts of the compute service
      * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
      *                     perform this operation in zero simulated time.
      * @return total core counts
      *
      */
    unsigned long ComputeService::getTotalNumCores(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_cores", simulate_it);

        unsigned long count = 0;
        for (auto const &x: dict) {
            count += static_cast<unsigned long>(x.second);
        }
        return count;
    }


    /**
     * @brief Get idle core counts for each of the compute service's host
     * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
     *                     perform this operation in zero simulated time.
     * @return the idle core counts (could be empty). Note that this doesn't
     *        mean that asking for these cores right now will mean immediate execution (since
     *        jobs may be pending and "ahead" in the queue, e.g., because they depend on current
     *        actions that are not using all available resources).
     *
     */
    std::map<std::string, unsigned long> ComputeService::getPerHostNumIdleCores(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_idle_cores", simulate_it);

        std::map<std::string, unsigned long> to_return;

        for (auto const &x: dict) {
            to_return.insert(std::make_pair(x.first, static_cast<unsigned long>(x.second)));
        }

        return to_return;
    }

    /**
     * @brief Get ram availability for each of the compute service's host
     * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
     *                     perform this operation in zero simulated time.
     * @return the ram availability map (could be empty)
     *
     */
    std::map<std::string, double> ComputeService::getPerHostAvailableMemoryCapacity(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("ram_availabilities", simulate_it);

        std::map<std::string, double> to_return;

        for (auto const &x: dict) {
            to_return.insert(std::make_pair(x.first, static_cast<double>(x.second)));
        }

        return to_return;
    }

    /**
     * @brief Get the total idle core count for all hosts of the compute service. Note that this doesn't
     *        mean that asking for these cores right will mean immediate execution (since
     *        jobs may be pending and "ahead" in the queue, e.g., because they depend on current
     *        actions that are not using all available resources).
     * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
     *                     perform this operation in zero simulated time.
     * @return total idle core count.
     *
     */
    unsigned long ComputeService::getTotalNumIdleCores(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("num_idle_cores", simulate_it);


        unsigned long count = 0;
        for (auto const &x: dict) {
            count += static_cast<unsigned long>(x.second);
        }
        return count;
    }

    /**
     * @brief Method to find out if, right now, the compute service has at least one host
     *        with some idle number of cores and some available RAM. Note that this doesn't
     *        mean that asking for these resources right will mean immediate execution (since
     *        jobs may be pending and "ahead" in the queue, e.g., because they depend on current
     *        actions that are not using all available resources).
     * @param num_cores: the desired number of cores
     * @param ram: the desired RAM
     * @return true if idle resources are available, false otherwise
     */
    bool ComputeService::isThereAtLeastOneHostWithIdleResources(unsigned long num_cores, sg_size_t ram) {
        assertServiceIsUp();

        // send an "info request" message to the daemon's commport
        auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

        this->commport->putMessage(new ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesRequestMessage(
                answer_commport,
                num_cores,
                ram,
                this->getMessagePayloadValue(
                        ComputeServiceMessagePayload::IS_THERE_AT_LEAST_ONE_HOST_WITH_AVAILABLE_RESOURCES_REQUEST_MESSAGE_PAYLOAD)));

        // Get the reply
        auto msg = answer_commport->getMessage<ComputeServiceIsThereAtLeastOneHostWithAvailableResourcesAnswerMessage>(
                this->network_timeout,
                "BareMetalComputeService::isThereAtLeastOneHostWithIdleResources(): received an");

        return msg->answer;
    }

    /**
    * @brief Get the per-core flop rate of the compute service's hosts
    * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
    *                     perform this operation in zero simulated time.
    * @return a list of flop rates in flop/sec
    *
    */
    std::map<std::string, double> ComputeService::getCoreFlopRate(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("flop_rates", simulate_it);

        std::map<std::string, double> to_return;
        for (auto const &x: dict) {
            to_return.insert(std::make_pair(x.first, x.second));
        }

        return to_return;
    }

    /**
    * @brief Get the RAM capacities for each of the compute service's hosts
    * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
    *                     perform this operation in zero simulated time.
    * @return a map of RAM capacities, indexed by hostname
    *
    */
    std::map<std::string, double> ComputeService::getMemoryCapacity(bool simulate_it) {
        std::map<std::string, double> dict;
        dict = this->getServiceResourceInformation("ram_capacities", simulate_it);

        std::map<std::string, double> to_return;

        for (auto const &x: dict) {
            to_return.insert(std::make_pair(x.first, x.second));
        }

        return to_return;
    }

    /**
     * @brief Get information about the compute service as a dictionary of vectors
     * @param key: the desired resource information (i.e., dictionary key) that's needed)
     * @param simulate_it: if true, simulate a message sent to the service to retrieve the information, otherwise
     *                     perform this operation in zero simulated time.
     * @return service information
     *
     */
    std::map<std::string, double> ComputeService::getServiceResourceInformation(const std::string &key, bool simulate_it) {
        assertServiceIsUp();

        if (simulate_it) {
            // send an "info request" message to the daemon's commport
            auto answer_commport = S4U_Daemon::getRunningActorRecvCommPort();

            this->commport->putMessage(new ComputeServiceResourceInformationRequestMessage(
                    answer_commport,
                    key,
                    this->getMessagePayloadValue(
                            ComputeServiceMessagePayload::RESOURCE_DESCRIPTION_REQUEST_MESSAGE_PAYLOAD)));

            // Get the reply
            auto msg = answer_commport->getMessage<ComputeServiceResourceInformationAnswerMessage>(
                    this->network_timeout,
                    "BareMetalComputeService::getServiceResourceInformation(): received an");
            return msg->info;
        } else {
            return this->constructResourceInformation(key);
        }
    }

    /**
     * @brief Get the total capacity of the compute service's scratch storage space
     * @return a size (in bytes)
     */
    double ComputeService::getTotalScratchSpaceSize() {
        // A scratch space SS is always created with a single mount point
        return this->scratch_space_storage_service ? this->scratch_space_storage_service->getTotalSpace() : 0.0;
    }

    /**
     * @brief Get the free space on the compute service's scratch storage space
     * @return a size (in bytes)
     */
    double ComputeService::getFreeScratchSpaceSize() {
        return this->scratch_space_storage_service->getTotalFreeSpace();
    }

    /**
    * @brief Get the compute service's scratch storage space
    * @return a pointer to the shared scratch space
    */
    std::shared_ptr<StorageService> ComputeService::getScratch() {
        return this->scratch_space_storage_service;
    }

    //    /**
    //    * @brief Get a shared pointer to the compute service's scratch storage space
    //    * @return a shared pointer to the shared scratch space
    //    */
    //    std::shared_ptr<StorageService> ComputeService::getScratchSharedPtr() {
    //        return this->scratch_space_storage_service_shared_ptr;
    //    }

    /**
    * @brief Checks if the compute service has a scratch space
    * @return true if the compute service has some scratch storage space, false otherwise
    */
    bool ComputeService::hasScratch() const {
        return (not this->scratch_space_mount_point.empty()) or (this->scratch_space_storage_service != nullptr);
    }

    /**
     * @brief Method the validates service-specific arguments (throws std::invalid_argument if invalid)
     * @param job: the job that's being submitted
     * @param service_specific_args: the service-specific arguments
     */
    void ComputeService::validateServiceSpecificArguments(const std::shared_ptr<CompoundJob> &job,
                                                          map<std::string, std::string> &service_specific_args) {
        throw std::runtime_error("ComputeService::validateServiceSpecificArguments(): should be overridden in compute service implementation");
    }


    /**
     * @brief Method to validate that a job's use of the scratch space if ok. Throws exception if not.
     * @param service_specific_args: the job;'s service-specific arguments (useful for some services)
     */
    void ComputeService::validateJobsUseOfScratch(std::map<std::string, std::string> &service_specific_args) {
        if (not this->hasScratch()) {
            throw std::invalid_argument("Compute service (" + this->getName() + ") does not have scratch space");
        }
    }


}// namespace wrench

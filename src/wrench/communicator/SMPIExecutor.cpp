
#include <algorithm>
#include <utility>

#include <wrench/logging/TerminalOutput.h>
#include "wrench/communicator/SMPIExecutor.h"
#include "wrench/simulation/Simulation.h"
#include "smpi/smpi.h"
#include "simgrid/s4u.hpp"


WRENCH_LOG_CATEGORY(wrench_core_smpi_executor, "Log category for SMPIExecutor");

namespace wrench {

    /**
     * @brief A Functor class for an MPI_Alltoall participant
     */
    class MPI_Alltoall_participant {
    public:
        /**
         * @brief Constructor
         * @param data_size: number of data_size to send/recv
         */
        explicit MPI_Alltoall_participant(int data_size) : data_size(data_size) {}

        /**
	 * @brief The actor's main method
	 */
        void operator()() {

            MPI_Init();
            void *data = SMPI_SHARED_MALLOC(data_size * data_size);
            MPI_Alltoall(data, data_size, MPI_CHAR, data, data_size, MPI_CHAR, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);
            MPI_Finalize();
        }

    private:
        int data_size;
    };


    /**
     * @brief Method to perform an SMPI Alltoall
     * @param hosts: list of hosts
     * @param data_size: size in data_size of each message sent/received
     */
    void SMPIExecutor::performAlltoall(std::vector<simgrid::s4u::Host *> &hosts,
                                       int data_size) {
        // Start actors to do an MPI_AllToAll
        std::string instance_id = "MPI_AllToAll_" + std::to_string(simgrid::s4u::this_actor::get_pid());
        SMPI_app_instance_start(instance_id.c_str(),
                                MPI_Alltoall_participant(data_size),
                                hosts);
        SMPI_app_instance_join(instance_id);
    }


    /**
     * @brief A Functor class for an MPI_Barrier participant
     */
    class MPI_Barrier_participant {
    public:
        /**
         * @brief Constructor
         */
        MPI_Barrier_participant() = default;

        /**
	 * @brief The actor's main method
	 */
        void operator()() {

            MPI_Init();
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();
        }
    };


    /**
     * @brief Method to perform an SMPI Barrier
     * @param hosts: list of hosts
     */
    void SMPIExecutor::performBarrier(std::vector<simgrid::s4u::Host *> &hosts) {
        std::string instance_id = "MPI_Barrier_" + std::to_string(simgrid::s4u::this_actor::get_pid());
        // Start actors to do an MPI_AllToAll
        SMPI_app_instance_start(instance_id.c_str(),
                                MPI_Barrier_participant(),
                                hosts);
        SMPI_app_instance_join(instance_id);
    }


    /**
     * @brief A Functor class for an MPI_Bcast participant
     */
    class MPI_Bcast_participant {
    public:
        /**
         * @brief Constructor
         * @param data_size: number of data_size to send/recv
         */
        explicit MPI_Bcast_participant(int data_size) : data_size(data_size) {}

        /**
	 * @brief The actor's main method
	 */
        void operator()() {

            MPI_Init();
            void *data = SMPI_SHARED_MALLOC(data_size);
            MPI_Bcast(data, data_size, MPI_CHAR, 0, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);
            MPI_Finalize();
        }

    private:
        int data_size;
    };

    /**
     * @brief Method to perform an SMPI Bcast
     * @param hosts: list of hosts
     * @param root_host: the host on which the root of the broadcast runs
     * @param data_size: size in data_size of each message sent/received
     */
    void SMPIExecutor::performBcast(std::vector<simgrid::s4u::Host *> &hosts,
                                    simgrid::s4u::Host *root_host,
                                    int data_size) {

        // Make sure that the root_host is the first host in the list of hosts, so that it's always rank 0
        auto it = std::find(hosts.begin(), hosts.end(), root_host);
        std::iter_swap(hosts.begin(), it);

        // Start actors to do an MPI_BCast
        std::string instance_id = "MPI_Bcast_" + std::to_string(simgrid::s4u::this_actor::get_pid());
        SMPI_app_instance_start(instance_id.c_str(),
                                MPI_Bcast_participant(data_size),
                                hosts);
        SMPI_app_instance_join(instance_id);
    }

}// namespace wrench

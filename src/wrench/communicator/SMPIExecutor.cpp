
#include <algorithm>
#include <utility>

#include <smpi/smpi.h>
#include <simgrid/s4u.hpp>

#include "wrench/logging/TerminalOutput.h"
#include "wrench/communicator/SMPIExecutor.h"
#include "wrench/simulation/Simulation.h"


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
        explicit MPI_Alltoall_participant(const int data_size) : data_size(data_size) {}

        /**
	 * @brief The actor's main method
	 */
        void operator()() {
            MPI_Init();
            int num_ranks;
            MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
            const auto ranks = static_cast<std::size_t>(num_ranks);
            const auto bytes_per_peer = static_cast<std::size_t>(data_size);
            if (bytes_per_peer > std::numeric_limits<std::size_t>::max() / ranks) {
                throw std::overflow_error("All-to-all buffer size exceeds size_t");
            }
            const auto buffer_size = ranks * bytes_per_peer;
            void* send_buffer = SMPI_SHARED_MALLOC(buffer_size);
            // void* receive_buffer = SMPI_SHARED_MALLOC(buffer_size);
            void *receive_buffer = send_buffer; // This should be fine given SMPI's semantics

            MPI_Alltoall(send_buffer, data_size, MPI_CHAR, receive_buffer, data_size, MPI_CHAR, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(send_buffer);
            // SMPI_SHARED_FREE(receive_buffer);
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
    void SMPIExecutor::performAlltoall(const std::vector<simgrid::s4u::Host *> &hosts,
                                       const int data_size) {
        // Start actors to do an MPI_AllToAll
        const std::string instance_id = "MPI_AllToAll_" + std::to_string(simgrid::s4u::this_actor::get_pid());
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
        void operator()() const
        {
            MPI_Init();
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();
        }
    };


    /**
     * @brief Method to perform an SMPI Barrier
     * @param hosts: list of hosts
     */
    void SMPIExecutor::performBarrier(const std::vector<simgrid::s4u::Host *> &hosts) {
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
        explicit MPI_Bcast_participant(int data_size, int root_rank) : data_size(data_size), root_rank(root_rank) {}

        /**
	 * @brief The actor's main method
	 */
        void operator()() {
            MPI_Init();
            void *data = SMPI_SHARED_MALLOC(data_size);
            MPI_Bcast(data, data_size, MPI_CHAR, root_rank, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);
            MPI_Finalize();
        }

    private:
        int data_size;
        int root_rank;
    };

    /**
     * @brief Method to perform an SMPI Bcast
     * @param hosts: list of hosts
     * @param root_rank: the rank of host on which the root of the broadcast runs
     * @param data_size: size in data_size of each message sent/received
     */
    void SMPIExecutor::performBcast(const std::vector<simgrid::s4u::Host *>& hosts,
                                    const int root_rank,
                                    const int data_size) {

        // Start actors to do an MPI_BCast
        std::string instance_id = "MPI_Bcast_" + std::to_string(simgrid::s4u::this_actor::get_pid());
        SMPI_app_instance_start(instance_id.c_str(),
                                MPI_Bcast_participant(data_size, root_rank),
                                hosts);
        SMPI_app_instance_join(instance_id);
    }

}// namespace wrench

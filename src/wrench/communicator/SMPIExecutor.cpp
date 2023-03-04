
#include <algorithm>

#include <wrench/logging/TerminalOutput.h>
#include "wrench/communicator/SMPIExecutor.h"
#include "wrench/simulation/Simulation.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "smpi/smpi.h"
#include "simgrid/s4u.hpp"



WRENCH_LOG_CATEGORY(wrench_core_smpi_executor, "Log category for SMPIExecutor");

namespace wrench {

    /**
     * @brief A Functor class for an MPI_Alltoall participant
     */
    class MPI_Alltoall_partipant {
    public:
        /**
         * @brief Constructor
         * @param data_size: number of data_size to send/recv
         * @param notify_mailbox: mailbox to notify when done
         */
        MPI_Alltoall_partipant(int data_size, simgrid::s4u::Mailbox *notify_mailbox) : data_size(data_size), notify_mailbox(notify_mailbox) {}

        void operator ()() {

            MPI_Init();
            void *data = SMPI_SHARED_MALLOC(data_size * data_size);
            MPI_Alltoall(data, data_size, MPI_CHAR, data, data_size, MPI_CHAR, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);
            MPI_Finalize();

            // Notify my creator of completion
            S4U_Mailbox::putMessage(notify_mailbox, new SimulationMessage(0));
        }

    private:
        int data_size;
        simgrid::s4u::Mailbox *notify_mailbox;
    };


    /**
     * @brief Method to perform an SMPI Alltoall
     * @param hosts: list of hosts
     * @param data_size: size in data_size of each message sent/received
     */
    void SMPIExecutor::performAlltoall(std::vector<simgrid::s4u::Host *> &hosts,
                                       int data_size) {
        // Create a mailbox to receive notifications of completion
        auto mailbox = S4U_Mailbox::getTemporaryMailbox();
        // Start actors to do an MPI_AllToAll
        SMPI_app_instance_start(("MPI_AllToAll_" + std::to_string(simgrid::s4u::this_actor::get_pid())).c_str(),
                                MPI_Alltoall_partipant(data_size, mailbox),
                                hosts);
        // Wait for all of those actors to be done
        for (int i=0; i < hosts.size(); i++) {
            S4U_Mailbox::getMessage(mailbox);
        }
    }


    /**
     * @brief A Functor class for an MPI_Barrier participant
     */
    class MPI_Barrier_partipant {
    public:
        /**
         * @brief Constructor
         * @param notify_mailbox: mailbox to notify when done
         */
        explicit MPI_Barrier_partipant(simgrid::s4u::Mailbox *notify_mailbox) : notify_mailbox(notify_mailbox) {}

        void operator ()() {

            MPI_Init();
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Finalize();

            // Notify my creator of completion
            S4U_Mailbox::putMessage(notify_mailbox, new SimulationMessage(0));
        }

    private:
        simgrid::s4u::Mailbox *notify_mailbox;
    };


    /**
     * @brief Method to perform an SMPI Barrier
     * @param hosts: list of hosts
     */
    void SMPIExecutor::performBarrier(std::vector<simgrid::s4u::Host *> &hosts) {
        // Create a mailbox to receive notifications of completion
        auto mailbox = S4U_Mailbox::getTemporaryMailbox();
        // Start actors to do an MPI_AllToAll
        SMPI_app_instance_start(("MPI_Barrier_" + std::to_string(simgrid::s4u::this_actor::get_pid())).c_str(),
                                MPI_Barrier_partipant(mailbox),
                                hosts);
        // Wait for all of those actors to be done
        for (int i=0; i < hosts.size(); i++) {
            S4U_Mailbox::getMessage(mailbox);
        }
    }


    /**
     * @brief A Functor class for an MPI_Bcast participant
     */
    class MPI_Bcast_partipant {
    public:
        /**
         * @brief Constructor
         * @param data_size: number of data_size to send/recv
         * @param notify_mailbox: mailbox to notify when done
         */
        MPI_Bcast_partipant(int data_size, simgrid::s4u::Mailbox *notify_mailbox) : data_size(data_size), notify_mailbox(notify_mailbox) {}

        void operator ()() {

            MPI_Init();
            void *data = SMPI_SHARED_MALLOC(data_size);
            MPI_Bcast(data, data_size, MPI_CHAR, 0, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);
            MPI_Finalize();

            // Notify my creator of completion
            S4U_Mailbox::putMessage(notify_mailbox, new SimulationMessage(0));
        }

    private:
        int data_size;
        simgrid::s4u::Mailbox *notify_mailbox;
    };

    /**
     * @brief Method to perform an SMPI Bcast
     * @param hosts: list of hosts
     * @param data_size: size in data_size of each message sent/received
     */
    void SMPIExecutor::performBcast(std::vector<simgrid::s4u::Host *> &hosts,
                                    simgrid::s4u::Host *root_host,
                                    int data_size) {
        // Create a mailbox to receive notifications of completion
        auto mailbox = S4U_Mailbox::getTemporaryMailbox();

        // Make sure that the root_host is the first host in the list of hosts, so that it's always rank 0
        auto it = std::find(hosts.begin(), hosts.end(), root_host);
        std::iter_swap(hosts.begin(), it);

        // Start actors to do an MPI_BCast
        SMPI_app_instance_start(("MPI_Bcast_" + std::to_string(simgrid::s4u::this_actor::get_pid())).c_str(),
                                MPI_Bcast_partipant(data_size, mailbox),
                                hosts);
        // Wait for all of those actors to be done
        for (int i=0; i < hosts.size(); i++) {
            S4U_Mailbox::getMessage(mailbox);
        }
    }

}
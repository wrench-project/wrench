
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
     * @brief A Functor class for an MPI-All-to-All participant
     */
    class AllToAllParticipant {
    public:
        AllToAllParticipant(int bytes, simgrid::s4u::Mailbox *notify_mailbox) : bytes(bytes), notify_mailbox(notify_mailbox) {}

        void operator ()() {

            MPI_Init();

            int rank;
            int size;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Comm_size(MPI_COMM_WORLD, &size);

//            std::vector<char> out(bytes * size);
//            std::vector<char> in(bytes * size);
//            MPI_Alltoall(out.data(), bytes, MPI_CHAR, in.data(), bytes, MPI_CHAR, MPI_COMM_WORLD);

            void *data = SMPI_SHARED_MALLOC(bytes * size);
            MPI_Alltoall(data, bytes, MPI_CHAR, data, bytes, MPI_CHAR, MPI_COMM_WORLD);
            SMPI_SHARED_FREE(data);

            MPI_Finalize();

            // Notify my creator of completion
            S4U_Mailbox::putMessage(notify_mailbox, new SimulationMessage(0));
        }

    private:
        int bytes;
        simgrid::s4u::Mailbox *notify_mailbox;
    };


    /**
     * @brief Method to perform an SMPI AllToAll
     * @param hosts: list of hosts
     * @param data_size: size in bytes of each message sent/received
     */
    void SMPIExecutor::performAllToAll(const std::vector<simgrid::s4u::Host *> &hosts,
                                       int data_size) {
        // Create a mailbox to receive notifications of completion
        auto mailbox = S4U_Mailbox::getTemporaryMailbox();
        // Start actors to do an MPI_AllToAll
        SMPI_app_instance_start("MPI_alltoall", AllToAllParticipant(data_size, mailbox), hosts);
        // Wait for all of those actors to be done
        for (int i=0; i < hosts.size(); i++) {
            S4U_Mailbox::getMessage(mailbox);
        }
    }

}
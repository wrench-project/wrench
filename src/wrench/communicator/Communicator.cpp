

#include <wrench/logging/TerminalOutput.h>
#include "wrench/communicator/Communicator.h"
#include "wrench/simulation//Simulation.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "smpi/smpi.h"
#include "simgrid/s4u.hpp"

WRENCH_LOG_CATEGORY(wrench_core_communicator, "Log category for Communicator");


namespace wrench {

    /**
     * @brief Factory method to construct a communicator
     * @param size the size of the communicator (# of processes)
     * @return a shared pointer to a communicator
     */
    std::shared_ptr<Communicator> Communicator::createCommunicator(unsigned long size) {
        if (size <= 1) {
            throw std::invalid_argument("Communicator::createCommunicator(): invalid argument");
        }
        return std::shared_ptr<Communicator>(new Communicator(size));
    }

    /**
     * @brief Get the number of processes participating in the communicator
     * @return a number of processes
     */
    unsigned long Communicator::getNumRanks() {
        return this->rank_to_mailbox.size();
    }

    /**
     * @brief Join the communicator and obtain a rank, which will block until all other communicator
     *        participants have joined (or just obtain the rank and return immediately if already joined).
     * @return a rank
     */
    unsigned long Communicator::join() {
        return this->join(this->rank_to_mailbox.size());
    }

    /**
     * @brief Join the communicator with a particular rank, which will block until all other communicator
     *        participants have joined (or return immediately if already joined).
     * @param desired_rank: the desired rank
     * @return the desired rank
     */
    unsigned long Communicator::join(unsigned long desired_rank) {
        auto my_pid = simgrid::s4u::this_actor::get_pid();

        if (desired_rank >= this->size) {
            throw std::invalid_argument("Communicator::join(): invalid arguments");
        }
        if (this->rank_to_mailbox.find(desired_rank) != this->rank_to_mailbox.end()) {
            if (this->actor_to_rank.find(my_pid) != this->actor_to_rank.end()) {
                return this->actor_to_rank[my_pid];
            } else {
                throw std::invalid_argument("Communicator::join(): rank already used by another participant");
            }
        }

        this->rank_to_mailbox[desired_rank] = S4U_Mailbox::getTemporaryMailbox();
        this->actor_to_rank[my_pid] = desired_rank;

        if (this->size > this->rank_to_mailbox.size()) {
            simgrid::s4u::this_actor::suspend();
        } else {
            for (auto const &item: this->actor_to_rank) {
                if (item.first != my_pid) {
                    simgrid::s4u::Actor::by_pid(item.first)->resume();
                }
            }
        }

        return desired_rank;
    }

    /**
     * @brief Perform asynchronous sends and receives
     * @param sends: the specification of all outgoing communications as <rank, volume in bytes> pairs
     * @param num_receives: the number of expected received (from any source)
     */
    void Communicator::sendAndReceive(const std::map<unsigned long, double> &sends, int num_receives) {
        this->sendReceiveAndCompute(sends, num_receives, 0);
    }


    /**
     * @brief Perform concurrent asynchronous sends, receives, and a computation
     * @param sends: the specification of all outgoing communications as <rank, volume in bytes> pairs
     * @param num_receives: the number of expected received (from any source)
     * @param flops: the number of floating point operations to compute
     */
    void Communicator::sendReceiveAndCompute(const std::map<unsigned long, double> &sends, int num_receives, double flops) {
        auto my_pid = simgrid::s4u::this_actor::get_pid();

        if (this->actor_to_rank.find(my_pid) == this->actor_to_rank.end()) {
            throw std::invalid_argument("Communicator::communicate(): Calling process is not part of this communicator");
        }
        // Post all the sends
        std::vector<std::shared_ptr<S4U_PendingCommunication>> posted_sends;
        for (auto const &send_operation: sends) {
            auto dst_mailbox = this->rank_to_mailbox[send_operation.first];
            posted_sends.push_back(S4U_Mailbox::iputMessage(dst_mailbox, new wrench::SimulationMessage(send_operation.second)));
        }
        // Post the computation (if any)
        simgrid::s4u::ExecPtr computation = nullptr;
        if (flops > 0) {
            computation = simgrid::s4u::this_actor::exec_init(flops);
        }

        // Do all the synchronous receives
        for (int i = 0; i < num_receives; i++) {
            S4U_Mailbox::getMessage(this->rank_to_mailbox[this->actor_to_rank[my_pid]]);
        }
        // Wait for all the sends
        for (auto const &posted_send: posted_sends) {
            posted_send->wait();
        }
        // Wait for the computation
        if (computation) {
            computation->wait();
        }
    }

    /**
     * @brief Barrier method (all participants wait for each other)
     */
    void Communicator::barrier() {
        auto my_pid = simgrid::s4u::this_actor::get_pid();
        static unsigned long count = 0;
        if (this->actor_to_rank.find(my_pid) == this->actor_to_rank.end()) {
            throw std::invalid_argument("Communicator::barrier(): Calling process is not part of this communicator");
        }

        count++;
        if (count < this->size) {
            simgrid::s4u::this_actor::suspend();
        } else {
            count = 0;
            for (auto const &item: this->actor_to_rank) {
                if (item.first != my_pid) {
                    simgrid::s4u::Actor::by_pid(item.first)->resume();
                }
            }
        }
    }

    /**
     * @brief Destructor
     */
    Communicator::~Communicator() {

        for (auto const &item: this->rank_to_mailbox) {
            S4U_Mailbox::retireTemporaryMailbox(item.second);
        }
    }

    /**
     * An MPI AllToAll method, which uses SimGrid' SMPI underneath
     *
     * @param bytes: the number of bytes in each exchanged message
     */
    void Communicator::MPI_AllToAll(double bytes) {
        if (bytes < 1.0) {
            throw std::runtime_error("Communicator::MPI_AllToAll(): invalid argument (should be >= 1.0)");
        }

        // Global synchronization
        this->barrier();
        auto my_pid = simgrid::s4u::this_actor::get_pid();
        static unsigned long count = 0;
        if (this->actor_to_rank.find(my_pid) == this->actor_to_rank.end()) {
            throw std::invalid_argument("Communicator::MPI_AllToAll(): Calling process is not part of this communicator");
        }
        count++;
        if (count < this->size) {
            simgrid::s4u::this_actor::suspend();
        } else {
            // I am the last arrived process and will be doing the entire operation with temp actors

            // Gather the list of hosts
            std::vector<simgrid::s4u::Host *> hosts;
            for (auto const &item: this->actor_to_rank) {
                hosts.push_back(simgrid::s4u::Actor::by_pid(item.first)->get_host());
            }

            // Create all tmp actors that will do the AllToAll
            Communicator::performAllToAll(hosts, (int) bytes);

            // Resume everyone
            for (auto const &item: this->actor_to_rank) {
                if (item.first != my_pid) {
                    simgrid::s4u::Actor::by_pid(item.first)->resume();
                }
            }
        }
    }

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

            std::vector<char> out(bytes * size);
            std::vector<char> in(bytes * size);
//            WRENCH_INFO("I am an MPI Process with Rank %d Calling MPI_Alltoall", rank);
            MPI_Alltoall(out.data(), bytes, MPI_CHAR, in.data(), bytes, MPI_CHAR, MPI_COMM_WORLD);

//            WRENCH_INFO("I am done with the call to MPI_AllToAll");
            MPI_Finalize();

            S4U_Mailbox::putMessage(notify_mailbox, new SimulationMessage(0));
        }
    private:
        int bytes;
        simgrid::s4u::Mailbox *notify_mailbox;
    };


    void Communicator::performAllToAll(const std::vector<simgrid::s4u::Host *> &hosts, int bytes) {
        // Create a mailbox to receive notifications of completion
        auto mailbox = S4U_Mailbox::getTemporaryMailbox();
        // Start actors to do an MPI_AllToAll
        SMPI_app_instance_start("MPI_alltoall", AllToAllParticipant(bytes, mailbox), hosts);
        // Wait for all of those actors to be done
        for (int i=0; i < hosts.size(); i++) {
            S4U_Mailbox::getMessage(mailbox);
        }
    }

}
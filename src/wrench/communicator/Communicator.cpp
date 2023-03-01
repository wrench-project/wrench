

#include "wrench/communicator/Communicator.h"
#include "wrench/simgrid_S4U_util/S4U_Mailbox.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"

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
        if (desired_rank >= this->size) {
            throw std::invalid_argument("Communicator::join(): invalid arguments");

        }
        if (this->rank_to_mailbox.find(desired_rank) != this->rank_to_mailbox.end()) {
            if (this->actor_to_rank.find(simgrid::s4u::this_actor::get_pid()) != this->actor_to_rank.end()) {
                return this->actor_to_rank[simgrid::s4u::this_actor::get_pid()];
            } else {
                throw std::invalid_argument("Communicator::join(): rank already used by another participant");
            }
        }

        this->rank_to_mailbox[desired_rank] = S4U_Mailbox::getTemporaryMailbox();
        this->actor_to_rank[simgrid::s4u::this_actor::get_pid()] = desired_rank;

        if (this->size > this->rank_to_mailbox.size()) {
            simgrid::s4u::this_actor::suspend();
        } else {
            for (auto const &item : this->actor_to_rank) {
                if (item.first != simgrid::s4u::this_actor::get_pid()) {
                    simgrid::s4u::Actor::by_pid(item.first)->resume();
                }
            }
        }

        return desired_rank;
    }


    /**
     * @brief Perform a communication step
     * @param sends: the specification of all outgoing communications as <rank, volume in bytes> pairs
     * @param num_receives: the number of expected received (from any source)
     */
    void Communicator::communicate(const std::map<unsigned long, double>& sends, int num_receives) {
        // Post all the sends
        std::vector<std::shared_ptr<S4U_PendingCommunication>> posted_sends;
        for (auto const &send_operation : sends) {
            auto dst_mailbox = this->rank_to_mailbox[send_operation.first];
            posted_sends.push_back(S4U_Mailbox::iputMessage(dst_mailbox, new wrench::SimulationMessage(send_operation.second)));
        }
        // Do all the synchronous receives
        for (int i=0; i < num_receives; i++) {
            S4U_Mailbox::getMessage(this->rank_to_mailbox[this->actor_to_rank[simgrid::s4u::this_actor::get_pid()]]);
        }
        // Wait for all the sends
        for (auto const &posted_send : posted_sends) {
            posted_send->wait();
        }
    }

    /**
     * @brief Destructor
     */
    Communicator::~Communicator() {
        for (auto const &item : this->rank_to_mailbox) {
            S4U_Mailbox::retireTemporaryMailbox(item.second);
        }
    }

}

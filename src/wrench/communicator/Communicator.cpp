

#include <wrench/logging/TerminalOutput.h>
#include "wrench/communicator/Communicator.h"
#include "wrench/communicator/SMPIExecutor.h"
#include "wrench/simulation//Simulation.h"
#include "wrench/simgrid_S4U_util/S4U_CommPort.h"
#include "wrench/simgrid_S4U_util/S4U_PendingCommunication.h"
#include "simgrid/s4u.hpp"
#include "smpi/smpi.h"

WRENCH_LOG_CATEGORY(wrench_core_communicator, "Log category for Communicator");


namespace wrench {

    /**
     * @brief Destructor
     */
    Communicator::~Communicator() {
        for (auto const &item: this->rank_to_commport) {
            S4U_CommPort::retireTemporaryCommPort(item.second);
        }
    }

    /**
     * @brief Factory method to construct a communicator
     * @param size the size of the communicator (# of processes)
     * @return a shared pointer to a communicator
     */
    std::shared_ptr<Communicator> Communicator::createCommunicator(const unsigned long size) {
        if (size <= 1) {
            throw std::invalid_argument("Communicator::createCommunicator(): invalid argument");
        }
        S4U_Simulation::enableSMPI();
        return std::shared_ptr<Communicator>(new Communicator(size));
    }

    /**
     * @brief Get the number of processes participating in the communicator
     * @return a number of processes
     */
    unsigned long Communicator::getNumRanks() const
    {
        return this->rank_to_commport.size();
    }

    /**
     * @brief Join the communicator and obtain a rank, which will block until all other communicator
     *        participants have joined (or just obtain the rank and return immediately if already joined).
     * @return a rank
     */
    unsigned long Communicator::join() {
        return this->join(this->rank_to_commport.size());
    }

    /**
     * @brief Join the communicator with a particular rank, which will block until all other communicator
     *        participants have joined (or return immediately if already joined).
     * @param desired_rank: the desired rank
     * @return the desired rank
     */
    unsigned long Communicator::join(const unsigned long desired_rank) {
        const auto my_pid = simgrid::s4u::this_actor::get_pid();

        if (desired_rank >= this->size) {
            throw std::invalid_argument("Communicator::join(): invalid arguments");
        }
        if (this->rank_to_commport.find(desired_rank) != this->rank_to_commport.end()) {
            if (this->actor_to_rank.find(my_pid) != this->actor_to_rank.end()) {
                return this->actor_to_rank[my_pid];
            } else {
                throw std::invalid_argument("Communicator::join(): rank already used by another participant");
            }
        }

        this->rank_to_commport[desired_rank] = S4U_CommPort::getTemporaryCommPort();
        this->rank_to_host[desired_rank] = simgrid::s4u::this_actor::get_host();
        this->actor_to_rank[my_pid] = desired_rank;
        this->participating_hosts.push_back(simgrid::s4u::this_actor::get_host());

        if (this->size > this->rank_to_commport.size()) {
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
     * @brief Perform asynchronous sends and receives operations, using standard WRENCH/SimGrid point to
     *        point communications.
     * @param sends: the specification of all outgoing communications as <rank, volume in bytes> pairs
     * @param num_receives: the number of expected received (from any source)
     */
    void Communicator::sendAndReceive(const std::map<unsigned long, sg_size_t> &sends, const int num_receives) {
        this->sendReceiveAndCompute(sends, num_receives, 0);
    }


    /**
     * @brief Perform concurrent asynchronous sends, receives, and a computation, using standard WRENCH/SimGrid point to
     *        point communications
     * @param sends: the specification of all outgoing communications as <rank, volume in bytes> pairs
     * @param num_receives: the number of expected received (from any source)
     * @param flops: the number of floating point operations to compute
     */
    void Communicator::sendReceiveAndCompute(const std::map<unsigned long, sg_size_t> &sends, const int num_receives, const double flops) {
        const auto my_pid = simgrid::s4u::this_actor::get_pid();

        if (this->actor_to_rank.find(my_pid) == this->actor_to_rank.end()) {
            throw std::invalid_argument("Communicator::communicate(): Calling process is not part of this communicator");
        }
        // Post all the sends
        std::vector<std::shared_ptr<S4U_PendingCommunication>> posted_sends;
        for (auto const &send_operation: sends) {
            const auto dst_commport = this->rank_to_commport[send_operation.first];
            posted_sends.push_back(dst_commport->iputMessage(new wrench::SimulationMessage(send_operation.second)));
        }
        // Post the computation (if any)
        simgrid::s4u::ExecPtr computation = nullptr;
        if (flops > 0) {
            computation = simgrid::s4u::this_actor::exec_init(flops);
        }

        // Do all the synchronous receives
        for (int i = 0; i < num_receives; i++) {
            this->rank_to_commport[this->actor_to_rank[my_pid]]->getMessage();
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
     * @brief Barrier method (all participants wait for each other), using standard WRENCH/SimGrid mechanisms
     */
    void Communicator::barrier() {
        const auto my_pid = simgrid::s4u::this_actor::get_pid();
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
     * @brief Perform an MPI AllToAll collective, using SimGrid's SMPI implementation
     *
     * @param bytes: the number of bytes in each message sent/received
     * @param config: the SMPI config option
     */
    void Communicator::MPI_Alltoall(const sg_size_t bytes, std::string config) {
        if (bytes < 1) {
            throw std::runtime_error("Communicator::MPI_Alltoall(): invalid argument (should be >= 1)");
        }
        this->performSMPIOperation("Alltoall", this->participating_hosts, nullptr, bytes, "smpi/alltoall:" + std::move(config));
    }

    /**
     * @brief Perform an MPI cast collective, using SimGrid's SMPI implementation
     *
     * @param root_rank: the rank of the root of the broadcast
     * @param bytes: the number of bytes in each message sent/received
     * @param config: the SMPI config option
     */
    void Communicator::MPI_Bcast(const int root_rank, const sg_size_t bytes, std::string config) {
        if ((bytes < 1) or (root_rank < 0) or (root_rank >= static_cast<int>(this->size))) {
            throw std::runtime_error("Communicator::MPI_Bcast(): invalid argument");
        }
        this->performSMPIOperation("Bcast", this->participating_hosts, this->rank_to_host[root_rank], bytes, "smpi/bcast:" + std::move(config));
    }

    /**
     * @brief Perform an MPI Barrier, using SimGrid's SMPI implementation
     * 
     * @param config: the SMPI config option
     */
    void Communicator::MPI_Barrier(std::string config) {
        this->performSMPIOperation("Barrier", this->participating_hosts, nullptr, 0, "smpi/barrier:" + std::move(config));
    }


    /**
     * @brief Helper method to perform SMPI Operations
     * @param op_name: operation name
     * @param hosts: hosts involved
     * @param root_host: root hosts (nullptr if none)
     * @param bytes: data size in bytes (0 if none)
     * @param config: the SMPI config option
     */
    void Communicator::performSMPIOperation(const std::string &op_name,
                                            std::vector<simgrid::s4u::Host *> &hosts,
                                            simgrid::s4u::Host *root_host,
                                            sg_size_t bytes,
                                            const std::string &config) {


        S4U_Simulation::enableSMPI();

        // Global synchronization
        this->barrier();
        const auto my_pid = simgrid::s4u::this_actor::get_pid();
        static unsigned long count = 0;
        if (this->actor_to_rank.find(my_pid) == this->actor_to_rank.end()) {
            throw std::invalid_argument("Communicator::MPI_AllToAll(): Calling process is not part of this communicator");
        }
        count++;
        if (count < this->size) {
            simgrid::s4u::this_actor::suspend();
        } else {
            // I am the last arrived process and will be doing the entire operation with temp actors
            // Set the configuration
            simgrid::s4u::Engine::set_config(config);

            if (op_name == "Alltoall") {
                SMPIExecutor::performAlltoall(hosts, bytes);
            } else if (op_name == "Bcast") {
                SMPIExecutor::performBcast(hosts, root_host, bytes);
            } else if (op_name == "Barrier") {
                SMPIExecutor::performBarrier(hosts);
            } else {
                throw std::runtime_error("Communicator::performSMPIOperation(): Internal error - unknown operation");
            }

            // Resume everyone
            for (auto const &item: this->actor_to_rank) {
                if (item.first != my_pid) {
                    simgrid::s4u::Actor::by_pid(item.first)->resume();
                }
            }
        }
    }


}// namespace wrench



#ifndef WRENCH_COMMUNICATOR_H
#define WRENCH_COMMUNICATOR_H

#include <memory>
#include <map>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    /**
     * @brief A class that implements a communicator (ala MPI) abstractions
     */
    class Communicator {

    public:
        static std::shared_ptr<Communicator> createCommunicator(unsigned long size);

        unsigned long join();
        unsigned long join(unsigned long desired_rank);
        unsigned long getNumRanks() const;
        void barrier();
        void sendAndReceive(const std::map<unsigned long, sg_size_t> &sends, int num_receives);
        void sendReceiveAndCompute(const std::map<unsigned long, sg_size_t> &sends, int num_receives, double flops);

        void MPI_Alltoall(sg_size_t bytes, std::string config = "ompi");
        void MPI_Bcast(int root_rank, sg_size_t bytes, std::string config = "ompi");
        void MPI_Barrier(std::string config = "ompi");

        ~Communicator();

    protected:
        /**
	 * @brief Constructor
	 *
	 * @param size: the communicator's size in number of processes
	 */
        explicit Communicator(unsigned long size) : size(size){};

    private:
        unsigned long size;
        std::map<aid_t, unsigned long> actor_to_rank;
        std::map<unsigned long, S4U_CommPort *> rank_to_commport;
        std::map<unsigned long, simgrid::s4u::Host *> rank_to_host;
        std::vector<simgrid::s4u::Host *> participating_hosts;

        void performSMPIOperation(const std::string &op_name,
                                  std::vector<simgrid::s4u::Host *> &hosts,
                                  simgrid::s4u::Host *root_host,
                                  sg_size_t bytes,
                                  const std::string &config);
    };

}// namespace wrench


#endif//WRENCH_COMMUNICATOR_H

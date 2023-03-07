

#ifndef WRENCH_COMMUNICATOR_H
#define WRENCH_COMMUNICATOR_H

#include <memory>
#include <map>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class Communicator {

    public:
        static std::shared_ptr<Communicator> createCommunicator(unsigned long size);

        unsigned long join();
        unsigned long join(unsigned long desired_rank);
        unsigned long getNumRanks();
        void barrier();
        void sendAndReceive(const std::map<unsigned long, double> &sends, int num_receives);
        void sendReceiveAndCompute(const std::map<unsigned long, double> &sends, int num_receives, double flops);

        void MPI_Alltoall(double bytes, std::string config = "ompi");
        void MPI_Bcast(int root_rank, double bytes, std::string config = "ompi");
        void MPI_Barrier(std::string config = "ompi");

        ~Communicator();

    protected:
        explicit Communicator(unsigned long size) : size(size){};

    private:
        unsigned long size;
        std::map<aid_t, unsigned long> actor_to_rank;
        std::map<unsigned long, simgrid::s4u::Mailbox *> rank_to_mailbox;
        std::map<unsigned long, simgrid::s4u::Host *> rank_to_host;
        std::vector<simgrid::s4u::Host *> participating_hosts;

        void performSMPIOperation(const std::string &op_name,
                                  std::vector<simgrid::s4u::Host *> &hosts,
                                  simgrid::s4u::Host *root_hosts,
                                  int bytes,
                                  const std::string &config);
    };

}// namespace wrench


#endif//WRENCH_COMMUNICATOR_H

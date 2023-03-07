
#ifndef WRENCH_SMPI_EXECUTOR_H
#define WRENCH_SMPI_EXECUTOR_H

namespace wrench {

    /**********************/
    /** @cond INTERNAL    */
    /**********************/

    class SMPIExecutor {

    public:
        static void performAlltoall(std::vector<simgrid::s4u::Host *> &hosts, int data_size);
        static void performBcast(std::vector<simgrid::s4u::Host *> &hosts, simgrid::s4u::Host *root_host, int data_size);
        static void performBarrier(std::vector<simgrid::s4u::Host *> &hosts);
    };

    /**********************/
    /** @endcond          */
    /**********************/

}// namespace wrench

#endif//WRENCH_SMPI_EXECUTOR_H

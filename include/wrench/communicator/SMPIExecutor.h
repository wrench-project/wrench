
#ifndef WRENCH_SMPI_EXECUTOR_H
#define WRENCH_SMPI_EXECUTOR_H

namespace wrench {

    /**********************/
    /** @cond INTERNAL    */
    /**********************/

    class SMPIExecutor {

    public:
        static void performAlltoall(const std::vector<simgrid::s4u::Host *> &hosts, int data_size);
        static void performBarrier(const std::vector<simgrid::s4u::Host *> &hosts);
    };

    /**********************/
    /** @endcond          */
    /**********************/

}

#endif//WRENCH_SMPI_EXECUTOR_H

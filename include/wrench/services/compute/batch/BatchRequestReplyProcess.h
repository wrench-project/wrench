//
// Created by Suraj Pandey on 10/17/17.
//

#ifndef WRENCH_BATCHREQUESTREPLYPROCESS_H
#define WRENCH_BATCHREQUESTREPLYPROCESS_H


#include <wrench/services/Service.h>

namespace wrench {

    class BatchRequestReplyProcess:public Service {
    public:
        BatchRequestReplyProcess(std::string hostname,std::string self_port, std::string sched_port);

    private:
        BatchRequestReplyProcess(std::string hostname, std::string self_port, std::string sched_port, std::string suffix);
        std::string hostname;

        int main() override;

        std::string self_port;
        std::string sched_port;

        bool processNextMessage();
    };
}


#endif //WRENCH_BATCHREQUESTREPLYPROCESS_H

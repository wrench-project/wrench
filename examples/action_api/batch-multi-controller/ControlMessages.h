
#ifndef CONTROLMESSAGES_H
#define CONTROLMESSAGES_H

#include <wrench-dev.h>

#define CONTROL_MESSAGE_SIZE 1024  // Size in bytes

namespace wrench {
    /**
     * Message to send a job request to a batch service controller
     */
    class JobRequestMessage : public ExecutionControllerCustomEventMessage {
        public:
        /**
         *
         * @param name job name
         * @param num_compute_nodes number of compute nodes
         * @param runtime job runtime
         * @param can_forward whether the job can be forwarded to another batch service controller
         */
        JobRequestMessage(const std::string& name,
                          const int num_compute_nodes,
                          const int runtime,
                          const bool can_forward) : ExecutionControllerCustomEventMessage(CONTROL_MESSAGE_SIZE),
                                              _name(name), _num_compute_nodes(num_compute_nodes), _runtime(runtime), _can_forward(can_forward) {}

        std::string _name;
        int _num_compute_nodes;
        int _runtime;
        bool _can_forward;
    };

    /**
     * Message to send a job completion notification
     */
    class JobNotificationMessage : public ExecutionControllerCustomEventMessage {
    public:
        /**
         *
         * @param name job name
         */
        JobNotificationMessage(const std::string& name) : ExecutionControllerCustomEventMessage(CONTROL_MESSAGE_SIZE),
                                              _name(name) {}

        std::string _name;
    };

}

#endif //CONTROLMESSAGES_H

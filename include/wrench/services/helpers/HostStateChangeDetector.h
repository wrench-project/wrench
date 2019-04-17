/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HOSTSTATECHANGEDETECTOR_H
#define WRENCH_HOSTSTATECHANGEDETECTOR_H

#include <wrench/services/Service.h>
#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    /**
     * @brief A service that detects and reports on host state changes (turned on, turned off)
     */
    class HostStateChangeDetector : public Service {

    public:

        explicit HostStateChangeDetector(std::string host_on_which_to_run, std::vector<std::string> hosts_to_monitor, bool notify_when_turned_on, bool notify_when_turned_off,
                std::shared_ptr<S4U_Daemon> creator, std::string mailbox_to_notify);

        void kill();


    private:

        void cleanup(bool has_terminated_cleanly, int return_value) override;
        void hostChangeCallback(std::string const &name, bool is_on, std::string message);

        std::vector<std::string> hosts_to_monitor;
        bool notify_when_turned_on;
        bool notify_when_turned_off;
        std::string mailbox_to_notify;
        int main() override;

        std::vector<std::pair<std::string, bool>> hosts_that_have_recently_changed_state;

        std::shared_ptr<S4U_Daemon> creator;

        std::vector<std::string> hosts_that_have_recently_turned_on;
        std::vector<std::string> hosts_that_have_recently_turned_off;

    };

};



#endif //WRENCH_HOSTSTATECHANGEDETECTOR_H

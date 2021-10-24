/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundatioff, either versioff 3 of the License, or
 * (at your optioff) any later versioff.
 */


#include "wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h"
#include "wrench/services/helper_services/host_state_change_detector/HostStateChangeDetectorMessage.h"
#include "wrench/simgrid_S4U_util/S4U_VirtualMachine.h"

#include <wrench/simulation/Simulation.h>
#include <wrench-dev.h>
#include <wrench/services/helper_services/host_state_change_detector/HostStateChangeDetector.h>


WRENCH_LOG_CATEGORY(wrench_core_host_state_change_detector, "Log category for HostStateChangeDetector");


/**
 * @brief Cleanup method
 *
 * @param has_returned_from_main: whether main() returned
 * @param return_value: the return value (if main() returned)
 */
void wrench::HostStateChangeDetector::cleanup(bool has_returned_from_main, int return_value) {
    // Unregister the callback!
    simgrid::s4u::Host::on_state_change.disconnect(this->on_state_change_call_back_id);
    simgrid::s4u::Host::on_speed_change.disconnect(this->on_speed_change_call_back_id);

}


/**
 * @brief Constructor
 * @param host_on_which_to_run: hosts on which this service runs
 * @param hosts_to_monitor: the list of hosts to monitor
 * @param notify_when_turned_on: whether to send a notifications when hosts turn on
 * @param notify_when_turned_off: whether to send a notifications when hosts turn off
 * @param notify_when_speed_change: whether to send a notification when hosts change speed
 * @param creator: the service that created this service (when its creator dies, so does this service)
 * @param mailbox_to_notify: the mailbox to notify
 * @param property_list: a property list
 *
 */
wrench::HostStateChangeDetector::HostStateChangeDetector(std::string host_on_which_to_run,
                                                         std::vector<std::string> hosts_to_monitor,
                                                         bool notify_when_turned_on, bool notify_when_turned_off, bool notify_when_speed_change,
                                                         std::shared_ptr<S4U_Daemon> creator,
                                                         std::string mailbox_to_notify,
                                                         std::map<std::string, std::string> property_list) :
        Service(host_on_which_to_run, "host_state_change_detector", "host_state_change_detector") {
    this->hosts_to_monitor = hosts_to_monitor;
    this->notify_when_turned_on = notify_when_turned_on;
    this->notify_when_turned_off = notify_when_turned_off;
    this->notify_when_speed_change = notify_when_speed_change;
    this->mailbox_to_notify = mailbox_to_notify;
    this->creator = creator;

    // Set default and specified properties
    this->setProperties(this->default_property_values, std::move(property_list));

    // Connect my member method to the on_state_change signal from SimGrid regarding Hosts
    this->on_state_change_call_back_id = simgrid::s4u::Host::on_state_change.connect(
            [this](simgrid::s4u::Host const &h) {
                this->hostStateChangeCallback(h.get_name());
            });

    // Connect my member method to the on_speed_change signal from SimGrid regarding Hosts
    this->on_speed_change_call_back_id = simgrid::s4u::Host::on_speed_change.connect(
            [this](simgrid::s4u::Host const &h) {
                this->hostSpeedChangeCallback(h.get_name());
            });

}

void wrench::HostStateChangeDetector::hostStateChangeCallback(std::string const &hostname) {
    if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), hostname) !=
        this->hosts_to_monitor.end()) {
        auto host = simgrid::s4u::Host::by_name(hostname);
        this->hosts_that_have_recently_changed_state.push_back(std::make_pair(hostname, host->is_on()));
    }
}

void wrench::HostStateChangeDetector::hostSpeedChangeCallback(std::string const &hostname) {
    if (std::find(this->hosts_to_monitor.begin(), this->hosts_to_monitor.end(), hostname) !=
        this->hosts_to_monitor.end()) {
        auto host = simgrid::s4u::Host::by_name(hostname);
        double speed = host->get_speed();
        this->hosts_that_have_recently_changed_speed.push_back(std::make_pair(hostname, speed));
    }
}


int wrench::HostStateChangeDetector::main() {

    WRENCH_INFO("Starting");
    while (true) {
        if (creator->getState() == State::DOWN) {
            WRENCH_INFO("My Creator has terminated/died, so must I...");
            break;
        }
        // Sleeping for my monitoring period
        Simulation::sleep(this->getPropertyValueAsDouble(HostStateChangeDetectorProperty::MONITORING_PERIOD));

        // State Changes
        while (not this->hosts_that_have_recently_changed_state.empty()) {
            auto host_info = this->hosts_that_have_recently_changed_state.at(0);
            std::string hostname = std::get<0>(host_info);
            bool new_state_is_on = std::get<1>(host_info);
            bool new_state_is_off = not new_state_is_on;
            this->hosts_that_have_recently_changed_state.erase(this->hosts_that_have_recently_changed_state.begin());

            HostStateChangeDetectorMessage *msg;

            if (this->notify_when_turned_on && new_state_is_on) {
                msg = new HostHasTurnedOnMessage(hostname);
            } else if (this->notify_when_turned_off && new_state_is_off) {
                msg = new HostHasTurnedOffMessage(hostname);
            } else {
                continue;
            }

            WRENCH_INFO("Notifying mailbox '%s' that host '%s' has changed state", this->mailbox_to_notify.c_str(),
                        hostname.c_str());
            S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
        }

        // Speed Changes
        while (not this->hosts_that_have_recently_changed_speed.empty()) {
            auto host_info = this->hosts_that_have_recently_changed_speed.at(0);
            std::string hostname = std::get<0>(host_info);
            double new_speed = std::get<1>(host_info);
            this->hosts_that_have_recently_changed_speed.erase(this->hosts_that_have_recently_changed_speed.begin());

            HostStateChangeDetectorMessage *msg;

            if (this->notify_when_speed_change) {
                msg = new HostHasChangedSpeedMessage(hostname, new_speed);
            } else {
                continue;
            }

            WRENCH_INFO("Notifying mailbox '%s' that host '%s' has changed speed", this->mailbox_to_notify.c_str(),
                        hostname.c_str());
            S4U_Mailbox::dputMessage(this->mailbox_to_notify, msg);
        }


    }
    return 0;
}

/**
 * @brief Kill the service
 */
void wrench::HostStateChangeDetector::kill() {
    this->killActor();
}

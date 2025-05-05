/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#include <wrench/simgrid_S4U_util/S4U_Daemon.h>
#include "wrench/simgrid_S4U_util/S4U_DaemonActor.h"
#include <wrench/simgrid_S4U_util/S4U_CommPort.h>
#include <wrench/logging/TerminalOutput.h>
#include <boost/algorithm/string.hpp>
#include <memory>
#include <wrench/failure_causes/HostError.h>

#ifdef MESSAGE_MANAGER
#include <wrench/util/MessageManager.h>
#endif


WRENCH_LOG_CATEGORY(wrench_core_s4u_daemon, "Log category for S4U_Daemon");

#ifdef ACTOR_TRACKING_OUTPUT
std::unordered_map<std::string, unsigned long> num_actors;
#endif

namespace wrench {

    std::unordered_map<aid_t, S4U_CommPort *> S4U_Daemon::map_actor_to_recv_commport;
    // std::unordered_map<aid_t, std::set<simgrid::s4u::MutexPtr>> S4U_Daemon::map_actor_to_held_mutexes;

    int S4U_Daemon::num_non_daemonized_actors_running = 0;

    /**
     * @brief Constructor (daemon with a commport)
     *
     * @param hostname: the name of the host on which the daemon will run
     * @param process_name_prefix: the prefix of the name of the simulated process/actor
     */
    S4U_Daemon::S4U_Daemon(const std::string &hostname, const std::string &process_name_prefix) {
        if (not simgrid::s4u::Engine::is_initialized()) {
            throw std::runtime_error("Simulation must be initialized before services can be created");
        }

        if (S4U_Simulation::get_host_or_vm_by_name_or_null(hostname) == nullptr) {
            throw std::invalid_argument("S4U_Daemon::S4U_Daemon(): Unknown host or vm '" + hostname + "'");
        }

#ifdef ACTOR_TRACKING_OUTPUT
        this->process_name_prefix = "";
        std::vector<std::string> tokens;
        boost::split(tokens, process_name_prefix, boost::is_any_of("_"));
        for (auto t: tokens) {
            if (t.at(0) < '0' or t.at(0) > '9') {
                this->process_name_prefix += t + "_";
            }
        }

        if (num_actors.find(this->process_name_prefix) == num_actors.end()) {
            num_actors.insert(std::make_pair(this->process_name_prefix, 1));
        } else {
            num_actors[this->process_name_prefix]++;
        }

        for (auto a: num_actors) {
            std::cerr << a.first << ":" << a.second << "\n";
        }
        std::cerr << "---------------\n";
#endif

        this->state = S4U_Daemon::State::CREATED;
        this->daemon_lock = simgrid::s4u::Mutex::create();
        this->hostname = hostname;
        this->host = S4U_Simulation::get_host_or_vm_by_name(hostname);
        this->simulation_ = nullptr;
        unsigned long seq = S4U_CommPort::generateUniqueSequenceNumber();
        this->process_name = process_name_prefix + "_" + std::to_string(seq);

        this->has_returned_from_main_ = false;

        //        std::cerr << "IN DAEMON CONSTRUCTOR: " << this->process_name << "\n";
        this->commport = S4U_CommPort::getTemporaryCommPort();
        this->recv_commport = S4U_CommPort::getTemporaryCommPort();
        TRACK_OBJECT("actor");
    }

    /**
     * @brief Destructor
     */
    S4U_Daemon::~S4U_Daemon() {
        //        WRENCH_INFO("IN DAEMON DESTRUCTOR (%s, %p)'", this->getName().c_str(), this->s4u_actor.get());

#ifdef ACTOR_TRACKING_OUTPUT
        num_actors[this->process_name_prefix]--;
#endif
        UNTRACK_OBJECT("actor");
    }

    //    /**
    //     * @brief Release all mutexes that the running actor holds, if any
    //     */
    //    void S4U_Daemon::release_held_mutexes() {
    //        if (S4U_Daemon::map_actor_to_held_mutexes.find(simgrid::s4u::this_actor::get_pid()) != S4U_Daemon::map_actor_to_held_mutexes.end()) {
    //            for (auto const &mutex : S4U_Daemon::map_actor_to_held_mutexes[simgrid::s4u::this_actor::get_pid()]) {
    //                mutex->unlock();
    //            }
    //            S4U_Daemon::map_actor_to_held_mutexes.erase(simgrid::s4u::this_actor::get_pid());
    //        }
    //        S4U_Daemon::map_actor_to_held_mutexes[simgrid::s4u::this_actor::get_pid()].erase(this->daemon_lock);
    //    }

    /**
     * @brief Cleanup function called when the daemon terminates (for whatever reason). The
     *        default behavior is to throw an exception if the host is off. This method
     *        should be overridden in a daemons implements some fault-tolerant behavior, or
     *        is naturally tolerant.
     *
     * @param has_returned_from_main: whether the daemon returned from main() by itself
     * @param return_value: the return value from main (if has_terminated_cleanly is true)
     *
     */
    void S4U_Daemon::cleanup(bool has_returned_from_main, int return_value) {
        //        WRENCH_INFO("IN S4U_Daemon::cleanup() for %s", this->process_name.c_str());

        // Releasing held mutexes, if any
        //        this->release_held_mutexes();

        // Default behavior is to throw in case of any problem
        if ((not has_returned_from_main) and (not S4U_Simulation::isHostOn(hostname))) {
            throw std::runtime_error(
                    "S4U_Daemon::cleanup(): This daemon has died due to a failure of its host, but does not override cleanup() "
                    "(so that is can implement fault-tolerance or explicitly ignore fault) ");
        }
    }


    /**
     * @brief Get the daemon's state
     * @return a state
     */
    S4U_Daemon::State S4U_Daemon::getState() const {
        return this->state;
    }

    /**
     * @brief Delete the daemon's life-saver (use at your own risks, if you're not the Simulation class)
     */
    void S4U_Daemon::deleteLifeSaver() {
        if (this->life_saver) {
            // Necessary so that the Actor destructor
            // happens before the simulation engine shutdowns
            this->s4u_actor = nullptr;
            auto life_saver_ref = this->life_saver;
            this->life_saver = nullptr;
            delete life_saver_ref;
            // At this point the destructor may have be called,
            // so we can no longer access variables safely.
        }
    }

    /**
     * @brief Start the daemon
     *
     * @param daemonized: whether the S4U actor should be daemonized
     * @param auto_restart: whether the S4U actor should automatically restart after a host reboot
     *
     */
    void S4U_Daemon::startDaemon(bool daemonized, bool auto_restart) {

        // Check that there is a lifesaver
        if (not this->life_saver) {
            throw std::runtime_error(
                    "S4U_Daemon::startDaemon(): You must call createLifeSaver() before calling startDaemon()");
        }

        // Check that the simulation pointer is set
        if (not this->simulation_) {
            throw std::runtime_error(
                    "S4U_Daemon::startDaemon(): You must set the simulation field before calling startDaemon() (" +
                    this->getName() + ")");
        }

        // Check that the host is up!
        if (not S4U_Simulation::isHostOn(hostname)) {
            throw ExecutionException(std::make_shared<HostError>(hostname));
        }

        this->daemonized_ = daemonized;
        this->auto_restart_ = auto_restart;
        this->has_returned_from_main_ = false;
        //        this->commport = this->initial_commport + "_#" + std::to_string(this->num_starts);
        // Create the s4u_actor

        try {
            this->s4u_actor = simgrid::s4u::Engine::get_instance()->add_actor(this->process_name,
                                                          this->host,
                                                          S4U_DaemonActor(this));
        } catch (simgrid::Exception &) {
            throw std::runtime_error("S4U_Daemon::startDaemon(): SimGrid actor creation failed... shouldn't happen.");
        }

        //        WRENCH_INFO("STARTED ACTOR %p (%s)", this->s4u_actor.get(), this->process_name.c_str());

        // nullptr is returned if the host is off (not the current behavior in SimGrid... just paranoid here)
        if (this->s4u_actor == nullptr) {
            throw ExecutionException(std::make_shared<HostError>(hostname));
        }

        // This test here is critical. It's possible that the created actor above returns
        // right away, in which case calling daemonize() on it cases the calling actor to
        // terminate immediately. This is a weird simgrid::s4u behavior/bug, that may be
        // fixed at some point, but this test saves us for now.
        if (not this->has_returned_from_main_) {

            this->setupOnExitFunction();

            if (this->daemonized_) {
                this->s4u_actor->daemonize();
            } else {
                S4U_Daemon::num_non_daemonized_actors_running++;
            }

            if (this->auto_restart_) {
                this->s4u_actor->set_auto_restart(true);
            }
        }
    }

    /**
     * @brief Sets up the on_exit function for the actor
     */
    void S4U_Daemon::setupOnExitFunction() {
        this->s4u_actor->on_exit([this](bool failed) {
            if (not this->daemonized_) {
                S4U_Daemon::num_non_daemonized_actors_running--;
            }
            //          std::cerr << "*** NUM_NON_DAEMONIZED_ACTORS_RUNNING = " << S4U_Daemon::num_non_daemonized_actors_running << "\n";
            // Set state to down
            this->state = S4U_Daemon::State::DOWN;
            // Call cleanup
            this->cleanup(this->hasReturnedFromMain(), this->getReturnValue());
            // Free memory_manager_service for the object unless the service is set to auto-restart
            if ((S4U_Daemon::num_non_daemonized_actors_running == 0) or (not this->isSetToAutoRestart())) {
//                Service::increaseNumCompletedServicesCount();
#ifdef MESSAGE_MANAGER
                MessageManager::cleanUpMessages(this->commport);
#endif
                this->deleteLifeSaver();
            }
            return 0;
        });
    }


    /**
 * @brief Return the auto-restart status of the daemon
 * @return true or false
 */
    bool S4U_Daemon::isSetToAutoRestart() const {
        return this->auto_restart_;
    }

    /**
 * @brief Return the daemonized status of the daemon
 * @return true or false
 */
    bool S4U_Daemon::isDaemonized() const {
        return this->daemonized_;
    }

    /**
 * @brief Method that run's the user-defined main method (that's called by the S4U actor class)
 */
    void S4U_Daemon::runMainMethod() {
        //        this->commport = S4U_CommPort::getTemporaryCommPort();
        //        this->recv_commport = S4U_CommPort::getTemporaryCommPort();
        // Set the commport receiver
        // Causes Mailbox::put() to no longer implement a rendezvous communication.
        this->commport->s4u_mb->set_receiver(this->s4u_actor);
        //        this->recv_commport->set_receiver(this->s4u_actor);

        S4U_Daemon::map_actor_to_recv_commport[simgrid::s4u::this_actor::get_pid()] = this->recv_commport;
        //        S4U_Daemon::running_actors.insert(this->getSharedPtr<S4U_Daemon>());

        this->num_starts++;
        // Compute zero flop so that nothing happens
        // until the host has a >0 pstate
        S4U_Simulation::computeZeroFlop();
        this->state = State::UP;
        this->return_value_ = this->main();
        this->has_returned_from_main_ = true;
        this->state = State::DOWN;
        S4U_Daemon::map_actor_to_recv_commport.erase(simgrid::s4u::this_actor::get_pid());

        this->commport->s4u_mb->set_receiver(nullptr);
        S4U_CommPort::retireTemporaryCommPort(this->commport);
        this->recv_commport->s4u_mb->set_receiver(nullptr);
        S4U_CommPort::retireTemporaryCommPort(this->recv_commport);
        //        S4U_Daemon::running_actors.erase(this->getSharedPtr<S4U_Daemon>());


        // There is a potential problem here:
        //  - this could be the only non-daemonized actor
        //  - a non-daemonized actor currently holds a lock
        //  This would cause a "mutex is being destroyed but still held by an actor" exception
        // TODO: One idea: sleep a little bit (ugly, breaks one test!!)
        // TODO: One idea: lock/unlock a (new) global lock (breaks many tests!!)
        // S4U_Simulation::sleep(0.0000001);
        // S4U_Simulation::global_lock->lock();
        // S4U_Simulation::global_lock->unlock();
    }


    /**
 * @brief Kill the daemon/actor (does nothing if already dead)
 *
 * @return true if actor was killed, false if not (e.g., daemon was already dead)
 */
    bool S4U_Daemon::killActor() {
        // Do the kill only if valid actor and not already done
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main_)) {
            try {
                this->s4u_actor->kill();
            } catch (simgrid::Exception &) {
                throw std::runtime_error("simgrid::s4u::Actor::kill() failed... this shouldn't have happened");
            }
            // Really not sure why now we're setting this to true... but if we don't some tests fail.
            // Something to investigate at some point
            this->has_returned_from_main_ = true;
            return true;
        } else {
            return false;
        }
    }

    /**
 * @brief Suspend the daemon/actor.
 */
    void S4U_Daemon::suspendActor() const {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main_)) {
            this->s4u_actor->suspend();
        }
    }

    /**
 * @brief Resume the daemon/actor.
 */
    void S4U_Daemon::resumeActor() const {
        if ((this->s4u_actor != nullptr) && (not this->has_returned_from_main_)) {
            this->s4u_actor->resume();
        }
    }

    /**
 * @brief Join (i.e., wait for) the daemon.
 *
 * @return a pair <A,B> where A is boolean (true if the daemon terminated cleanly (i.e., main() returned), or false otherwise)
 *          and B is the int returned from main() (if main returned).
 */
    std::pair<bool, int> S4U_Daemon::join() const {
        if (this->s4u_actor != nullptr) {
            this->s4u_actor->join();
        } else {
            throw std::runtime_error("S4U_Daemon::join(): Fatal error: this->s4u_actor shouldn't be nullptr");
        }
        //        WRENCH_INFO("JOIN ON '%s' HAS RETURNED: %d %d", this->getName().c_str(), this->hasReturnedFromMain(), this->getReturnValue());
        return std::make_pair(this->hasReturnedFromMain(), this->getReturnValue());
    }

    /**
 * @brief Returns true if the daemon has returned from main() (i.e., not brutally killed)
 * @return The true or false
 */
    bool S4U_Daemon::hasReturnedFromMain() const {
        return this->has_returned_from_main_;
    }

    /**
 * @brief Returns the value returned by main() (if the daemon has returned from main)
 * @return The return value
 */
    int S4U_Daemon::getReturnValue() const {
        return this->return_value_;
    }

    /**
 * @brief Retrieve the process name
 * @return the name
 */
    std::string S4U_Daemon::getName() const {
        return this->process_name;
    }

    /**
 * @brief Create a life-saver for the daemon
 * @param reference
 */
    void S4U_Daemon::createLifeSaver(std::shared_ptr<S4U_Daemon> reference) {
        if (this->life_saver) {
            throw std::runtime_error("S4U_Daemon::createLifeSaver(): Lifesaver already created!");
        }
        this->life_saver = new S4U_Daemon::LifeSaver(reference);
    }

    /**
 * @brief Lock the daemon's lock
 */
    void S4U_Daemon::acquireDaemonLock() const {
        // S4U_Simulation::global_lock->lock();
        this->daemon_lock->lock();
        //        S4U_Daemon::map_actor_to_held_mutexes[simgrid::s4u::this_actor::get_pid()].insert(this->daemon_lock);
    }

    /**
 * @brief Unlock the daemon's lock
 */
    void S4U_Daemon::releaseDaemonLock() const {
        this->daemon_lock->unlock();
        // S4U_Simulation::global_lock->unlock();

        //        S4U_Daemon::map_actor_to_held_mutexes[simgrid::s4u::this_actor::get_pid()].erase(this->daemon_lock);
    }

    /**
     * @brief Get the service's simulation
     * @return a simulation
     */
    Simulation *S4U_Daemon::getSimulation() const {
        return this->simulation_;
    }

    /**
     * @brief Set the service's simulation
     * @param simulation: a simulation
     */
    void S4U_Daemon::setSimulation(Simulation *simulation) {
        this->simulation_ = simulation;
    }

    /**
     * @brief Return the running actor's recv commport
     * @return the commport
     */
    S4U_CommPort *S4U_Daemon::getRunningActorRecvCommPort() {

        return S4U_Daemon::map_actor_to_recv_commport[simgrid::s4u::this_actor::get_pid()];
    }

}// namespace wrench

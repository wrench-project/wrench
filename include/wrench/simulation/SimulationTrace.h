/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_SIMULATIONTRACE_H
#define WRENCH_SIMULATIONTRACE_H

#include <vector>
#include <map>
#include <cmath>
#include <cfloat>
#include <algorithm>

#include "wrench/simulation/SimulationTimestamp.h"

namespace wrench {


    /***********************/
    /** \cond              */
    /***********************/

    /** @brief A dummy top-level class derived by simulation output traces */
    class GenericSimulationTrace {

    public:
        virtual ~GenericSimulationTrace() {}
    };
    /***********************/
    /** \endcond           */
    /***********************/

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A template class to represent a trace of timestamps
     *
     * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
     */
    template<class T>
    class SimulationTrace : public GenericSimulationTrace {

    public:
        /**
         * @brief Append a timestamp to the trace
         *
         * @param timestamp: a pointer to a SimulationTimestamp<T> object
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         */
        void addTimestamp(SimulationTimestamp<T> *timestamp) {
            this->trace.push_back(timestamp);
        }


        /**
         * @brief Retrieve the trace as a vector of timestamps
         *
         * @tparam a particular SimulationTimestampXXXX class (defined in SimulationTimestampTypes.h)
         * @return a vector of pointers to SimulationTimestamp<T> objects
         */
        std::vector<SimulationTimestamp<T> *> getTrace() {
            return this->trace;
        }

        /**
         * @brief Destructor
         */
        ~SimulationTrace<T>() {
            for (auto &timestamp: this->trace) {
                delete timestamp;
            }
        }


    private:
        std::vector<SimulationTimestamp<T> *> trace;
    };


    /**
     * @brief A specialized class to represent a trace of SimulationTimestampPstateSet timestamps
     */
    template<>
    class SimulationTrace<SimulationTimestampPstateSet> : public GenericSimulationTrace {
    public:
        /**
         * @brief Append a SimulationTimestampPstateSet timestamp to the trace
         * @param new_timestamp: pointer to the timestamp
         */
        void addTimestamp(SimulationTimestamp<SimulationTimestampPstateSet> *new_timestamp) {
            auto hostname = new_timestamp->getContent()->getHostname();

            auto hostname_search = latest_timestamps_by_host.find(hostname);
            if (hostname_search == latest_timestamps_by_host.end()) {
                // no pstate timestamps associated to this hostname have been added yet
                this->trace.push_back(new_timestamp);
                this->latest_timestamps_by_host[hostname] = this->trace.size() - 1;
            } else {
                // a pstate timestamp associated to this host already exists
                SimulationTimestamp<SimulationTimestampPstateSet> *&latest_timestamp = this->trace[this->latest_timestamps_by_host[hostname]];

                // if the new_timestamp has the same date as the latest_timestamp, then the new_timestamp replaces
                // the latest_timestamp in the trace, else the new time_stamp is added to the trace and the map of latest
                // timestamps is updated to reflect this change
                if (std::fabs(new_timestamp->getDate() - latest_timestamp->getDate()) < DBL_EPSILON) {
                    std::swap(new_timestamp, latest_timestamp);

                    // now, latest_timestamp points to the contents of new_timestamp
                    // and new_timestamp points to the contents of latest_timestamp so
                    // we need to delete new_timestamp
                    delete new_timestamp;
                } else {
                    if (new_timestamp->getDate() > latest_timestamp->getDate()) {
                        this->trace.push_back(new_timestamp);
                        this->latest_timestamps_by_host[hostname] = this->trace.size() - 1;
                    } else {
                        throw std::runtime_error(
                                "SimulationTrace<SimulationTimestampPstateSet>::addTimestamp() timestamps out of order");
                    }
                }
            }
        }

        /**
         * @brief Retrieve the trace as a vector of SimulationTimestamp<SimulationTimestampPstateSet> timestamps
         * @return a vector of pointers to SimulationTimestamp<SimulationTimestampPstateSet> objects
         */
        std::vector<SimulationTimestamp<SimulationTimestampPstateSet> *> getTrace() {
            return this->trace;
        }

        /**
         * @brief Destructor
         */
        ~SimulationTrace<SimulationTimestampPstateSet>() {
            for (auto &timestamp: this->trace) {
                delete timestamp;
            }
        }

    private:
        std::map<std::string, size_t> latest_timestamps_by_host;
        std::vector<SimulationTimestamp<SimulationTimestampPstateSet> *> trace;
    };

    /***********************/
    /** \endcond INTERNAL  */
    /***********************/

};// namespace wrench


#endif//WRENCH_SIMULATIONTRACE_H

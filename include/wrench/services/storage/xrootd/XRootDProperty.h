//
// Created by jamcdonald on 6/6/2022.
//

#include "wrench/services/ServiceProperty.h"
#ifndef WRENCH_XROOTDPROPERTY_H
#define WRENCH_XROOTDPROPERTY_H
namespace wrench {
    namespace XRootD {
        /**
         * @brief Configurable service properties for a XRootD node
         */
        class Property : public ServiceProperty {

        public:
            /** @brief The overhead to handle a message, in flops **/
            DECLARE_PROPERTY_NAME(MESSAGE_OVERHEAD);
            /** @brief The overhead to handle a cache lookup, in flops **/
            DECLARE_PROPERTY_NAME(CACHE_LOOKUP_OVERHEAD);
            /** @brief The overhead to handle a search broadcast, in flops **/
            DECLARE_PROPERTY_NAME(SEARCH_BROADCAST_OVERHEAD);
            /** @brief The overhead to handle a cache update, in flops **/
            DECLARE_PROPERTY_NAME(UPDATE_CACHE_OVERHEAD);
            /** @brief The time an entry will remain in the cache before being erased, in seconds **/
            DECLARE_PROPERTY_NAME(CACHE_MAX_LIFETIME);
            /** @brief If set to "true", then the simulation of the XRootD search does not simulate all
             * control message sends/receives, but just those to the node that the search will find (which
             * can be determined in zero simulation time based on data structure lookups). This
             * makes the simulation faster but less accurate, which may not be desirable if the overhead
             * and/or network load of the control
             * messages involved in the search is an important part of the simulation (default is "false") **/
            DECLARE_PROPERTY_NAME(REDUCED_SIMULATION);
            /** @brief The ammount of time a supervisor should wait after a file request before sending a "file not found" message. Default: 30, Default unit: second. Example: "30", "20s", "100ms", etc.  */
            DECLARE_PROPERTY_NAME(FILE_NOT_FOUND_TIMEOUT);
        };

    }// namespace XRootD
}// namespace wrench
#endif//WRENCH_XROOTDPROPERTY_H

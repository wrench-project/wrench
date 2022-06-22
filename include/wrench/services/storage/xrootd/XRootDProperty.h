//
// Created by jamcdonald on 6/6/2022.
//

#include "wrench/services/ServiceProperty.h"
#ifndef WRENCH_XROOTDPROPERTY_H
#define WRENCH_XROOTDPROPERTY_H
namespace wrench {
    namespace XRootD{
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

        };

    }
}
#endif //WRENCH_XROOTDPROPERTY_H

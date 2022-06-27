#include <wrench/services/storage/xrootd/XRootDProperty.h>

namespace wrench {
    namespace XRootD{
        SET_PROPERTY_NAME(Property,MESSAGE_OVERHEAD);
        SET_PROPERTY_NAME(Property,CACHE_LOOKUP_OVERHEAD);
        SET_PROPERTY_NAME(Property,SEARCH_BROADCAST_OVERHEAD);
        SET_PROPERTY_NAME(Property,UPDATE_CACHE_OVERHEAD);
        SET_PROPERTY_NAME(Property,CACHE_MAX_LIFETIME);
    }
}
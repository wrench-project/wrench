//
// Created by jamcdonald on 1/17/23.
//

#ifndef WRENCH_STORAGESERVICEPROXYPROPERTY_H
#define WRENCH_STORAGESERVICEPROXYPROPERTY_H
#include "wrench/services/storage/StorageServiceProperty.h"


namespace wrench {

    /**
     * @brief Configurable properties for a StorageService
     */
    class StorageServiceProxyProperty : public StorageServiceProperty {

    public:
        /** @brief The overhead for handling just 1 message
         **/
        DECLARE_PROPERTY_NAME(MESSAGE_OVERHEAD);
        /** @brief The read behavior for an uncached message.
         * Due to a limitation in Simgrid, it is not generally possible to say "Copy a file to this server, and at the same time read the file from that server as bytes become avaliable"  As such, the copy to cache and the client read from cache have to be handled differently.  There are currently 3 supported ways, specified by this property.
         *  - CopyThenRead: The most basic thought of how to do this.  Copy the file to the cache, and then read the file from the cache once it has finished.  This method means the file will always arive in the cache when it should, and all internal network links are hit with the proper amount of congestion.  However, the file will arive at the client who requested it later than it should
         *
         *  - MagicRead:    This seeks to combat the problem with CopyThenRead by assuming the time to transfer the file from the cache to the client should be neglegable compared to the time to copy the file from remote to the cache.  As such, when the file copy is finished, all clients waiting on the file instantly receive it.  This means the file will always arive at the cache at the correct time, and should arrive at the client in roughly the correct time, but at the cost of network congestion accuracy
         *
         *  - ReadThrough:  This takes a slighly different approach, the file is transfered directly to the client, and as soon as the client has finished receiving it, the file instantly appears on the cache.  This offers the most accurate file-to-client time, however, the file-to-cache time is increased, and multiple client access happens later than otherwise expected as they have to wait for the cache to update.
         *  NOTE: There must be a network route between client and remote for ReadThrough to work, and for max accuracy, it should go through the Proxy host.
         *
         *  <table>
         *  <tr>
         *      <td></td>
         *      <td>CopyThenRead</td>
         *      <td>MagicRead</td>
         *      <td>ReadThrough</td>
         *  </tr>
         *  <tr>
         *      <td>File to cache time</td>
         *      <td>Accurate</td>
         *      <td>Accurate</td>
         *      <td>Overestimated</td>
         *  </tr>
         *  <tr>
         *      <td>File to client time</td>
         *      <td>Overestimated</td>
         *      <td>Probiably good</td>
         *      <td>Accurate</td>
         *  </tr>
         *  <tr>
         *      <td>Network Congestion</td>
         *      <td>Accurate</td>
         *      <td>Underestimated</td>
         *      <td>Accurate</td>
         *  </tr>
         *  <tr>
         *      <td>Parrallel behavior</td>
         *      <td>No change</td>
         *      <td>Amplifies weakness</td>
         *      <td>Decreased accuracy for secondary Host</td>
         *  </tr>
         *  <tr>
         *      <td>Required Network Links</td>
         *      <td>Proxy-Remote, Client-Proxy</td>
         *      <td>Proxy-Remote, Client-Proxy</td>
         *      <td>Client-Remote, Client-Proxy</td>
         *  </tr>
         *  </table>
         **/
        DECLARE_PROPERTY_NAME(UNCACHED_READ_METHOD);
    };

}// namespace wrench
#endif//WRENCH_STORAGESERVICEPROXYPROPERTY_H

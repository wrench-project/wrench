/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREGISTRYSERVICE_H
#define WRENCH_FILEREGISTRYSERVICE_H


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>

namespace wrench {

    class FileRegistryService : public S4U_DaemonWithMailbox {

    public:

        enum Property {
            /** The number of bytes in the control message
            * sent to the daemon to terminate it (default: 1024) **/
                    STOP_DAEMON_MESSAGE_PAYLOAD,
            /** The number of bytes in the control message
            * sent by the daemon to confirm it has terminate (default: 1024) **/
                    DAEMON_STOPPED_MESSAGE_PAYLOAD,
            /** The number of bytes in a request control message
             * sent to the daemon to request a list of file locations (default: 1024) **/
                    REQUEST_MESSAGE_PAYLOAD,
            /** The number of bytes per file location returned in an answer
             *   sent by the daemon to answer a file location request (default: 1024) */
                    ANSWER_MESSAGE_PAYLOAD,
            /** The number of bytes in the control message sent to the daemon
             * to cause it to remove an entry (default: 1024) */
                    REMOVE_ENTRY_PAYLOAD,
            /** The overhead, in seconds, of looking up entries for a file (default: 0.0s) */
                    LOOKUP_OVERHEAD,
        };


    private:

        std::map<FileRegistryService::Property, std::string> default_property_values =
                {{FileRegistryService::Property::STOP_DAEMON_MESSAGE_PAYLOAD,    "1024"},
                 {FileRegistryService::Property::DAEMON_STOPPED_MESSAGE_PAYLOAD, "1024"},
                 {FileRegistryService::Property::REQUEST_MESSAGE_PAYLOAD,        "1024"},
                 {FileRegistryService::Property::ANSWER_MESSAGE_PAYLOAD,         "1024"},
                 {FileRegistryService::Property::REMOVE_ENTRY_PAYLOAD,           "1024"},
                 {FileRegistryService::Property::LOOKUP_OVERHEAD,                "0.0"},
                };

    public:

        // Public Constructor
        FileRegistryService(std::string hostname,
                            std::map<FileRegistryService::Property, std::string> = {});

        /***********************/
        /** \cond DEVELOPER    */
        /***********************/

        enum State {
            UP,
            DOWN,
        };

        // Setting/Getting property
        void setProperty(FileRegistryService::Property, std::string);

        std::string getPropertyString(FileRegistryService::Property);

        std::string getPropertyValueAsString(FileRegistryService::Property);

        double getPropertyValueAsDouble(FileRegistryService::Property);

        void stop();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        FileRegistryService(std::string hostname,
                            std::map<FileRegistryService::Property, std::string> plist,
                            std::string suffix = "");

        std::map<FileRegistryService::Property, std::string> property_list;

        std::string hostname;

        int main();

        FileRegistryService::State state;

        bool processNextMessage();

    };


};


#endif //WRENCH_FILEREGISTRYSERVICE_H

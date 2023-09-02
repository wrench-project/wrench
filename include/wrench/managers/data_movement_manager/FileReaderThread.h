/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_FILEREADERTHREAD_H
#define WRENCH_FILEREADERTHREAD_H

#include <list>
#include <utility>

#include "wrench/simgrid_S4U_util/S4U_Daemon.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/simulation/SimulationMessage.h"

namespace wrench {


    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A helper daemon (co-located with a data movement manager)
     */
    class FileReaderThread : public Service {

    public:
        FileReaderThread(std::string hostname, simgrid::s4u::Mailbox *creator_mailbox,
                         std::shared_ptr<FileLocation> location,
                         double num_bytes);

    protected:
    private:
        int main() override;

        simgrid::s4u::Mailbox *creator_mailbox;
        std::shared_ptr<FileLocation> location;
        double num_bytes;
    };

    /***********************/
    /** \endcond            */
    /***********************/


}// namespace wrench


#endif//WRENCH_FILEREADERTHREAD_H

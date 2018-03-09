/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_DATAMOVEMENTMANAGER_H
#define WRENCH_DATAMOVEMENTMANAGER_H


#include "wrench/simgrid_S4U_util/S4U_Daemon.h"

namespace wrench {

    class Workflow;
    class WorkflowFile;
    class StorageService;
    class WMS;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief A helper daemon (co-located with a WMS) that handles data movement operations
     */
    class DataMovementManager : public S4U_Daemon {

    public:


        ~DataMovementManager();

        void stop();

        void kill();

        void initiateAsynchronousFileCopy(WorkflowFile *file, StorageService *src, StorageService *dst);

        void doSynchronousFileCopy(WorkflowFile *file, StorageService *src, StorageService *dst);

    protected:

        friend class WMS;

        DataMovementManager(WMS *wms);

    private:


        int main();

        WMS *wms = nullptr;

        bool processNextMessage();

    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_DATAMOVEMENTMANAGER_H

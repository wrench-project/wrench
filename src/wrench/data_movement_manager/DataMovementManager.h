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


#include <simgrid_S4U_util/S4U_DaemonWithMailbox.h>
#include <workflow/Workflow.h>

namespace wrench {

    class DataMovementManager : public S4U_DaemonWithMailbox {

    public:

        DataMovementManager(Workflow *workflow);

        ~DataMovementManager();

        void stop();

        void kill();

    private:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        int main();

        // Relevant workflow
        Workflow *workflow;

        std::string hostname;
        bool killed = false;

        /***********************/
        /** \endcond           */
        /***********************/
    };

    /***********************/
    /** \endcond            */
    /***********************/


};


#endif //WRENCH_DATAMOVEMENTMANAGER_H

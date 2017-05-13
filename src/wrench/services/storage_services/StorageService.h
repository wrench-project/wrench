/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_STORAGESERVICE_H
#define WRENCH_STORAGESERVICE_H


#include <string>
#include <workflow/WorkflowFile.h>
#include <set>
#include <services/Service.h>

namespace wrench {

    /***********************/
    /** \cond DEVELOPER   **/
    /***********************/

    class Simulation;  // Forward ref

    /**
     * @brief Abstract implementation of a storage service.
     */
    class StorageService : public Service {

    public:

        void stop();

        virtual void copyFile(WorkflowFile *file, StorageService *src) = 0;

        virtual void downloadFile(WorkflowFile *file) = 0;

        virtual void uploadFile(WorkflowFile *file) = 0;

        virtual void deleteFile(WorkflowFile *file) = 0;

        std::string getName();

        bool isUp();

        double getCapacity();

        double getFreeSpace();

        /***********************/
        /** \cond INTERNAL    **/
        /***********************/


        StorageService(std::string service_name_prefix, std::string mailbox_name_prefix, double capacity);

        void setStateToDown();

    protected:

        friend class Simulation;

        void setSimulation(Simulation *simulation);

        StorageService::State state;
        std::string service_name;
        Simulation *simulation;  // pointer to the simulation object

        void addFileToStorage(WorkflowFile *);

        void removeFileFromStorage(WorkflowFile *);


        std::set<WorkflowFile *> stored_files;
        double capacity;
        double occupied_space = 0;

    private:


    };

    /***********************/
    /** \endcond           */
    /***********************/

};


#endif //WRENCH_STORAGESERVICE_H

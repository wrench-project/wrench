/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_STORAGESERVICENONSIMULATIONOPERATIONS_H
#define WRENCH_STORAGESERVICENONSIMULATIONOPERATIONS_H

#include <string>
#include <set>

namespace wrench {

    /**
     * @brief The storage service base class
     */
    class StorageServiceNonSimulationOperations {

        class DataFile;

    public:
        /***********************/
        /** \cond DEVELOPER   **/
        /***********************/

        /** File lookup methods */
        virtual bool hasFile(const std::shared_ptr<DataFile> &file, const std::string &path = "/") = 0;


        /** File creation methods */
        virtual void createFile(const std::shared_ptr<DataFile> &file, const std::string &path = "/") = 0;

        /** File write date methods */
        virtual double getFileLastWriteDate(const std::shared_ptr<DataFile> &file, const std::string &path = "/") = 0;

        /** Service load methods */
        virtual double getLoad() = 0;

        /***********************/
        /** \endcond          **/
        /***********************/
    };

}// namespace wrench

#endif//WRENCH_STORAGESERVICENONSIMULATIONOPERATIONS_H

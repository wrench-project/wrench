/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DATAFILE_H
#define WRENCH_DATAFILE_H

#include <string>
#include <map>

namespace wrench {

    /**
     * @brief A data file used/produced by a WorkflowTask in a Workflow
     */
    class DataFile {

    public:
        double getSize();

        std::string getID();

    protected:
        friend class Simulation;
        DataFile(std::string id, double size);

        /** @brief File id/name **/
        std::string id;
        /** @brief File size in bytes **/
        double size;// in bytes
    };

};// namespace wrench

#endif//WRENCH_DATAFILE_H

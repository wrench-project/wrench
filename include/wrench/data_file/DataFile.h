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
#include <simgrid/forward.h>

namespace wrench {

    /**
     * @brief A data file used/produced by a WorkflowTask in a Workflow
     */
    class DataFile {

    public:
        [[nodiscard]] sg_size_t getSize() const;
        void setSize(sg_size_t size);
        [[nodiscard]] std::string getID() const;
        ~DataFile();

    protected:
        friend class Simulation;
        DataFile(std::string id, sg_size_t size);

        /** @brief File id/name **/
        std::string id;
        /** @brief File size in bytes **/
        sg_size_t size;// in bytes
    };

}// namespace wrench

#endif//WRENCH_DATAFILE_H

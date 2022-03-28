/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef WRENCH_BLOCK_H
#define WRENCH_BLOCK_H

#include <string>
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/

    /**
     * @brief A class that implements a "block" abstraction for memory management
     */
    class Block {

    public:
        Block(std::string fid, std::shared_ptr<FileLocation> location, double sz,
              double last_access, bool is_dirty, double dirty_time);

        Block(Block *blk);

        std::string getFileId();

        void setFileId(std::string &fid);

        //        std::string getMountpoint();

        //        void setMountpoint(std::string mountpoint);

        double getSize() const;

        void setSize(double size);

        double getLastAccess() const;

        void setLastAccess(double last_access);

        bool isDirty() const;

        void setDirty(bool is_dirty);

        double getDirtyTime() const;

        void setDirtyTime(double dirty_time);

        const std::shared_ptr<FileLocation> &getLocation() const;

        Block *split(double remaining);

    private:
        std::string file_id;
        //        std::string mountpoint;
        std::shared_ptr<FileLocation> location;
        double size;
        double last_access;
        bool dirty;
        double dirty_time;

        /***********************/
        /** \endcond           */
        /***********************/
    };

}// namespace wrench

#endif//WRENCH_BLOCK_H

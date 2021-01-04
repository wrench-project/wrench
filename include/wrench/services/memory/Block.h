//
// Created by Dzung Do on 2020-08-04.
//

#ifndef WRENCH_BLOCK_H
#define WRENCH_BLOCK_H

#include <string>
#include "wrench/services/storage/storage_helpers/FileLocation.h"

namespace wrench {

    class Block {

    public:
        Block(std::string fid, std::shared_ptr<FileLocation> location, double sz,
                     double last_access, bool is_dirty, double dirty_time);

        Block(Block *blk);

        std::string getFileId();

        void setFileId(std::string &fid);

        std::string getMountpoint();

//        void setMountpoint(std::string mountpoint);

        double getSize() const;

        void setSize(double sz);

        double getLastAccess() const;

        void setLastAccess(double lastAccess);

        bool isDirty() const;

        void setDirty(bool is_dirty);

        double getDirtyTime() const;

        void setDirtyTime(double time);

        const std::shared_ptr<FileLocation> &getLocation() const;

        Block* split(double remaining);

    private:
        std::string file_id;
//        std::string mountpoint;
        std::shared_ptr<FileLocation> location;
        double size;
        double last_access;
        bool dirty;
        double dirty_time;
    };

}

#endif //WRENCH_BLOCK_H

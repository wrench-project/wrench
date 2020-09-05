//
// Created by Dzung Do on 2020-08-04.
//

#ifndef WRENCH_BLOCK_H
#define WRENCH_BLOCK_H

#include <string>

namespace wrench {

    class Block {

    public:
        Block(std::string fn, std::string mount_point, double sz, double last_access, bool is_dirty, double dirty_time);

        Block(Block *blk);

        std::string getFilename();

        void setFilename(std::string &fn);

        std::string getMountpoint();

        void setMountpoint(std::string mountpoint);

        double getSize() const;

        void setSize(double sz);

        double getLastAccess() const;

        void setLastAccess(double lastAccess);

        bool isDirty() const;

        void setDirty(bool is_dirty);

        double getDirtyTime() const;

        void setDirtyTime(double dirtyTime);

        Block* split(double remaining);

    private:
        std::string filename;
        std::string mountpoint;
        double size;
        double last_access;
        bool dirty;
        double dirty_time;
    };

}

#endif //WRENCH_BLOCK_H

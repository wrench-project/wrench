//
// Created by Dzung Do on 2020-08-04.
//

#ifndef WRENCH_BLOCK_H
#define WRENCH_BLOCK_H

#include <string>

namespace wrench {

    class Block {

    public:
        Block(std::string filename, long size, long last_access, bool dirty);

        const std::string &getFilename() const;

        void setFilename(const std::string &filename);

        long getSize() const;

        void setSize(long size);

        long getLastAccess() const;

        void setLastAccess(long lastAccess);

        bool isDirty() const;

        void setDirty(bool dirty);

    private:
        std::string filename;
        long size;
        long last_access;
        bool dirty;
    };

}

#endif //WRENCH_BLOCK_H

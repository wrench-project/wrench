//
// Created by Dzung Do on 2020-08-04.
//

#ifndef WRENCH_BLOCK_H
#define WRENCH_BLOCK_H

#include <string>

namespace wrench {

    class Block {

    public:
        Block(std::string &fn, double sz, double last_access, bool is_dirty);

        const std::string &getFilename() const;

        void setFilename(const std::string &fn);

        double getSize() const;

        void setSize(double sz);

        double getLastAccess() const;

        void setLastAccess(double lastAccess);

        bool isDirty() const;

        void setDirty(bool is_dirty);

    private:
        std::string filename;
        double size;
        double last_access;
        bool dirty;
    };

}

#endif //WRENCH_BLOCK_H

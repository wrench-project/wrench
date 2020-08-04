//
// Created by Dzung Do on 2020-08-04.
//

#include <wrench/services/memory/Block.h>

namespace wrench {

    Block::Block(std::string filename, long size, long last_access, bool dirty) :
            filename(filename), size(size), last_access(last_access), dirty(dirty) {}

    const std::string &Block::getFilename() const {
        return filename;
    }

    void Block::setFilename(const std::string &filename) {
        Block::filename = filename;
    }

    long Block::getSize() const {
        return size;
    }

    void Block::setSize(long size) {
        Block::size = size;
    }

    long Block::getLastAccess() const {
        return last_access;
    }

    void Block::setLastAccess(long lastAccess) {
        last_access = lastAccess;
    }

    bool Block::isDirty() const {
        return dirty;
    }

    void Block::setDirty(bool dirty) {
        Block::dirty = dirty;
    }

}

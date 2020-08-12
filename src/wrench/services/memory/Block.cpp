//
// Created by Dzung Do on 2020-08-04.
//

#include <wrench/services/memory/Block.h>

namespace wrench {

    Block::Block(std::string &fn, double sz, double last_access, bool is_dirty) :
            filename(fn), size(sz), last_access(last_access), dirty(is_dirty) {}

    const std::string &Block::getFilename() const {
        return filename;
    }

    void Block::setFilename(const std::string &fn) {
        Block::filename = fn;
    }

    double Block::getSize() const {
        return size;
    }

    void Block::setSize(double sz) {
        Block::size = sz;
    }

    double Block::getLastAccess() const {
        return last_access;
    }

    void Block::setLastAccess(double lastAccess) {
        last_access = lastAccess;
    }

    bool Block::isDirty() const {
        return dirty;
    }

    void Block::setDirty(bool is_dirty) {
        Block::dirty = is_dirty;
    }

}

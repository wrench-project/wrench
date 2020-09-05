//
// Created by Dzung Do on 2020-08-04.
//

#include <wrench/services/memory/Block.h>

namespace wrench {

    Block::Block(std::string fn, std::string mount_point, double sz,
            double last_access, bool is_dirty, double dirty_time) :
            filename(fn), mountpoint(mount_point), size(sz),
            last_access(last_access), dirty(is_dirty), dirty_time(dirty_time) {}

    Block::Block(Block *blk) {
        this->filename = blk->getFilename();
        this->mountpoint = blk->getMountpoint();
        this->last_access = blk->getLastAccess();
        this->size = blk->getSize();
        this->dirty = blk->isDirty();
        this->dirty_time = blk->getDirtyTime();
    }

    std::string Block::getFilename() {
        return filename;
    }

    void Block::setFilename(std::string &fn) {
        Block::filename = fn;
    }

    std::string Block::getMountpoint() {
        return this->mountpoint;
    }

    void Block::setMountpoint(std::string mountpoint) {
        this->mountpoint = mountpoint;
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

    double Block::getDirtyTime() const {
        return dirty_time;
    }

    void Block::setDirtyTime(double dirtyTime) {
        dirty_time = dirtyTime;
    }

    Block *Block::split(double remaining) {

        if (remaining > this->size) remaining = this->size;
        if (remaining < 0) remaining = 0;

        Block *new_blk = new Block(this->filename, this->mountpoint, this->size - remaining, this->last_access,
                                   this->dirty, this->dirty_time);
        this->size = remaining;
        return new_blk;
    }

}

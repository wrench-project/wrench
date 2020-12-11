//
// Created by Dzung Do on 2020-08-04.
//

#include <wrench/services/memory/Block.h>

namespace wrench {

    Block::Block(std::string fid, std::shared_ptr<FileLocation> location, double sz,
            double last_access, bool is_dirty, double dirty_time) :
            file_id(fid), location(location), size(sz),
            last_access(last_access), dirty(is_dirty), dirty_time(dirty_time) {}

    Block::Block(Block *blk) {
        this->file_id = blk->getFileId();
//        this->mountpoint = blk->getMountpoint();
        this->location = blk->getLocation();
        this->last_access = blk->getLastAccess();
        this->size = blk->getSize();
        this->dirty = blk->isDirty();
        this->dirty_time = blk->getDirtyTime();
    }

    std::string Block::getFileId() {
        return this->file_id;
    }

    void Block::setFileId(std::string &fid) {
        this->file_id = fid;
    }

//    std::string Block::getMountpoint() {
//        return this->mountpoint;
//    }
//
//    void Block::setMountpoint(std::string mountpoint) {
//        this->mountpoint = mountpoint;
//    }

    double Block::getSize() const {
        return this->size;
    }

    void Block::setSize(double sz) {
        this->size = sz;
    }

    double Block::getLastAccess() const {
        return this->last_access;
    }

    void Block::setLastAccess(double lastAccess) {
        this->last_access = lastAccess;
    }

    bool Block::isDirty() const {
        return this->dirty;
    }

    void Block::setDirty(bool is_dirty) {
        this->dirty = is_dirty;
    }

    double Block::getDirtyTime() const {
        return this->dirty_time;
    }

    void Block::setDirtyTime(double dirtyTime) {
        this->dirty_time = dirtyTime;
    }

    Block *Block::split(double remaining) {

        if (remaining > this->size) remaining = this->size;
        if (remaining < 0) remaining = 0;

        Block *new_blk = new Block(this->file_id, this->location, this->size - remaining, this->last_access,
                                   this->dirty, this->dirty_time);
        this->size = remaining;
        return new_blk;
    }

    const std::shared_ptr<FileLocation> &Block::getLocation() const {
        return location;
    }

}

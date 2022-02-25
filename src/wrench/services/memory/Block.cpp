/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/memory/Block.h>

namespace wrench {

    /**
     * @brief Constructor
     * @param fid: file id
     * @param location: file location
     * @param sz: file size in bytes
     * @param last_access: time of last access
     * @param is_dirty: dirty status
     * @param dirty_time: dirty time
     */
    Block::Block(std::string fid, std::shared_ptr<FileLocation> location, double sz,
            double last_access, bool is_dirty, double dirty_time) :
            file_id(fid), location(location), size(sz),
            last_access(last_access), dirty(is_dirty), dirty_time(dirty_time) {}

    /**
     * @brief Constructor (that does a copy)
     * @param blk: a block
     */
    Block::Block(Block *blk) {
        this->file_id = blk->getFileId();
//        this->mountpoint = blk->getMountpoint();
        this->location = blk->getLocation();
        this->last_access = blk->getLastAccess();
        this->size = blk->getSize();
        this->dirty = blk->isDirty();
        this->dirty_time = blk->getDirtyTime();
    }

    /**
     * @brief Get the file id
     * @return the file id
     */
    std::string Block::getFileId() {
        return this->file_id;
    }

    /**
     * @brief Set the block's file id
     * @param fid: a file id
     */
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

    /**
     * @brief Get the block's size
     * @return a size in bytes
     */
    double Block::getSize() const {
        return this->size;
    }

    /**
     * @brief Set the block's size
     * @param size: a size in bytes
     */
    void Block::setSize(double size) {
        this->size = size;
    }

    /**
     * @brief Get the block's last access time
     * @return a date
     */
    double Block::getLastAccess() const {
        return this->last_access;
    }

    /**
     * @brief Set the block's last access time
     * @param last_access: a date
     */
    void Block::setLastAccess(double last_access) {
        this->last_access = last_access;
    }

    /**
     * @brief Get the block's dirty status
     * @return true or false
     */
    bool Block::isDirty() const {
        return this->dirty;
    }

    /**
     * @brief Set the block's dirty status
     * @param is_dirty: true or false
     */
    void Block::setDirty(bool is_dirty) {
        this->dirty = is_dirty;
    }

    /**
     * @brief Get the block's dirty time
     * @return a date
     */
    double Block::getDirtyTime() const {
        return this->dirty_time;
    }

    /**
     * @brief Set the block's dirty time
     * @param dirty_time: a date
     */
    void Block::setDirtyTime(double dirty_time) {
        this->dirty_time = dirty_time;
    }

    /**
     * @brief Split a block
     * @param remaining: a number of bytes
     * @return: a pointer to a new block
     */
    Block *Block::split(double remaining) {

        if (remaining > this->size) remaining = this->size;
        if (remaining < 0) remaining = 0;

        Block *new_blk = new Block(this->file_id, this->location, this->size - remaining, this->last_access,
                                   this->dirty, this->dirty_time);
        this->size = remaining;
        return new_blk;
    }

    /**
     * @brief Get the block's location
     * @return the block's location
     */
    const std::shared_ptr<FileLocation> &Block::getLocation() const {
        return location;
    }

}

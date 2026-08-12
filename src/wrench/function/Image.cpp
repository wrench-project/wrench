/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/function/Image.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(Image, "Log category for Serverless image");


namespace wrench {

    /**
     * @brief Constructor
     * @param name A name
     * @param location The location of the image (i.e., on some remote/authoritative repo)
     * @param ram_footprint The memory occupied by the resident, reusable portion of that image, in bytes
     */
    Image::Image(const std::string& name,
                 const std::shared_ptr<FileLocation>& location,
                 const sg_size_t ram_footprint) : _name(name), _location(location), _ram_footprint(ram_footprint) {
        _ram_file = Simulation::addFile(location->getFile()->getID() + "_RAM", _ram_footprint);
    }

    /**
     * @return The image's disk footprint
     */
    sg_size_t Image::getDiskFootprint() const {
        return _location->getFile()->getSize();
    }

    /**
     * @return The image file
     */
    std::shared_ptr<DataFile> Image::getFile() const {
        return _location->getFile();
    }

    /**
     * @return The image RAM space (as a file)
     */
    std::shared_ptr<DataFile> Image::getRAMFile() const {
        return _ram_file;
    }

} // namespace wrench

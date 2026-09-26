/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/function/ImageLayer.h"

#include <utility>
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(ImageLayer, "Log category for Serverless image layer");


namespace wrench {

    unsigned long ImageLayer::_creation_id_counter = 0;

    /**
     * @brief Constructor
     * @param name A name
     * @param location The location of the image layer file (i.e., on some remote/authoritative repo)
     * @param ram_footprint The memory occupied by the resident, reusable portion of that image layer, in bytes
     */
    ImageLayer::ImageLayer(std::string  name,
                 const std::shared_ptr<FileLocation>& location,
                 const sg_size_t ram_footprint) : _name(std::move(name)), _location(location), _ram_footprint(ram_footprint) {
        _creation_id = _creation_id_counter++;
        _ram_file = Simulation::addFile(location->getFile()->getID() + "_RAM", _ram_footprint);
    }

    /**
     * @brief Get the image's name
     * @return a name
     */
    std::string ImageLayer::getName() const {
        return _name;
    }

    /**
     * @brief Retrieve the layer's creation ID
     * @return the creation ID
     */
    unsigned long ImageLayer::getCreationId() const {
        return _creation_id;
    }

    /**
     * @brief Get the image's RAM footprint
     * @return a number of bytes
     */
    sg_size_t ImageLayer::getRAMFootprint() const {
        return _ram_footprint;
    }

    /**
     * @return The image's disk footprint
     */
    sg_size_t ImageLayer::getDiskFootprint() const {
        return _location->getFile()->getSize();
    }

    /**
     * @brief Get the image's file location
     * @return a location
     */
    std::shared_ptr<FileLocation> ImageLayer::getLocation() const {
        return _location;
    }

    /**
     * @return The image file
     */
    std::shared_ptr<DataFile> ImageLayer::getFile() const {
        return _location->getFile();
    }

    /**
     * @return The image RAM space (as a file)
     */
    std::shared_ptr<DataFile> ImageLayer::getRAMFile() const {
        return _ram_file;
    }

} // namespace wrench

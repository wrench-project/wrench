/**
 * Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/function/Image.h"

#include <utility>

#include "wrench/function/ImageLayer.h"
#include "wrench/logging/TerminalOutput.h"
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/simulation/Simulation.h"

WRENCH_LOG_CATEGORY(Image, "Log category for Serverless image");


namespace wrench {

    /**
     * @brief Constructor
     * @param name A name
     * @param layers The list of layers that comprise this image
     */
    Image::Image(std::string name,
                 const std::set<std::shared_ptr<ImageLayer>>& layers) : _name(std::move(name)), _layers(layers) {
    }

    /**
     * @brief Constructor
     * @param name A name
     * @param parent_image A parent image
     * @param layers The list of addition layers necessary for this image
     */
    Image::Image(std::string name,
                const std::shared_ptr<Image> &parent_image,
                const std::set<std::shared_ptr<ImageLayer>>& layers) : _name(std::move(name)) {
        for (auto const &layer: parent_image->getLayers()) {
            _layers.insert(layer);
        }
        for (auto const &layer: layers) {
            _layers.insert(layer);
        }
    }


    /**
     * @brief Get the image's name
     * @return a name
     */
    std::string Image::getName() const {
        return _name;
    }

    /**
     * @brief Get the image's layers
     * @return a list of layers
     */
    const std::set<std::shared_ptr<ImageLayer>>& Image::getLayers() const {
        return _layers;
    }

    /**
     * @brief Get the image's disk footprint
     * @return a number of bytes
     */
    sg_size_t Image::getDiskFootprint() const {
        sg_size_t size = 0;
        for (auto const &layer : _layers) {
            size += layer->getDiskFootprint();
        }
        return size;
    }

    /**
     * @brief Get the image's RAM footprint
     * @return a number of bytes
     */
    sg_size_t Image::getRAMFootprint() const {
        sg_size_t size = 0;
        for (auto const &layer : _layers) {
            size += layer->getRAMFootprint();
        }
        return size;
    }


} // namespace wrench

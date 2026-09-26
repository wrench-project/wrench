/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_IMAGE_H
#define WRENCH_IMAGE_H

#include <string>
#include <functional>
#include <set>
#include <memory>

#include <simgrid/forward.h>

#include <wrench/function/ImageLayer.h>


namespace wrench {
    class FileLocation;
    class ImageLayer;
    class DataFile;

    /**
     * @brief A class that implements the notion of an image for containers
     */
    class Image {
    public:
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] const std::set<std::shared_ptr<ImageLayer>>& getLayers() const;
        [[nodiscard]] sg_size_t getRAMFootprint() const;
        [[nodiscard]] sg_size_t getDiskFootprint() const;


    private:
        friend class FunctionManager;
        friend class Function;
        friend class Function;
        friend class Container;
        friend class ServerlessComputeNode;
        friend class ServerlessComputeService;

        Image(std::string  name,
              const std::set<std::shared_ptr<ImageLayer>>& layers);
        Image(std::string  name,
              const std::shared_ptr<Image> &parent_image,
              const std::set<std::shared_ptr<ImageLayer>>& layers);

        std::string _name;
        std::set<std::shared_ptr<ImageLayer>> _layers;
    };
} // namespace wrench

#endif // WRENCH_IMAGE_H

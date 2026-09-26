/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_IMAGE_LAYER_H
#define WRENCH_IMAGE_LAYER_H

#include <string>
#include <functional>
#include <memory>

#include <simgrid/forward.h>


namespace wrench {
    class FileLocation;
    class DataFile;
    class Image;

    /**
     * @brief A class that implements the notion of an image layer for containers
     */
    class ImageLayer {
    public:
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] unsigned long getCreationId() const;
        [[nodiscard]] sg_size_t getRAMFootprint() const;
        [[nodiscard]] sg_size_t getDiskFootprint() const;
        [[nodiscard]] std::shared_ptr<FileLocation> getLocation() const;
        [[nodiscard]] std::shared_ptr<DataFile> getFile() const;
        [[nodiscard]] std::shared_ptr<DataFile> getRAMFile() const;

    private:
        friend class FunctionManager;
        friend class Function;
        friend class Function;
        friend class Container;
        friend class ServerlessComputeNode;
        friend class ServerlessComputeService;

        ImageLayer(std::string  name,
              const std::shared_ptr<FileLocation>& location,
              sg_size_t ram_footprint);


        static unsigned long _creation_id_counter;

        std::string _name;
        unsigned long _creation_id;
        std::shared_ptr<FileLocation> _location;
        sg_size_t _ram_footprint;
        std::shared_ptr<DataFile> _ram_file;
    };
} // namespace wrench

#endif // WRENCH_IMAGE_LAYER_H

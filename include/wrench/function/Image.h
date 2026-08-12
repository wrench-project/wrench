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
#include <memory>

#include <simgrid/forward.h>


namespace wrench {
    class FileLocation;
    class DataFile;

    /**
     * @brief A class that implements the notion of a function that
     *        can be invoked at a serverless compute service
     */
    class Image {
    public:
        [[nodiscard]] std::string getName() const { return _name; }
        [[nodiscard]] sg_size_t getRAMFootprint() const { return _ram_footprint; }
        [[nodiscard]] sg_size_t getDiskFootprint() const;
        [[nodiscard]] std::shared_ptr<FileLocation> getLocation() const { return _location; }
        [[nodiscard]] std::shared_ptr<DataFile> getFile() const;

    private:
        friend class FunctionManager;
        friend class RegisteredFunction;
        friend class Function;
        friend class Container;
        friend class ServerlessComputeNode;
        friend class ServerlessComputeService;

        Image(const std::string& name,
              const std::shared_ptr<FileLocation>& location,
              sg_size_t ram_foot_print);

        [[nodiscard]] std::shared_ptr<DataFile> getRAMFile() const;

        std::string _name;
        std::shared_ptr<FileLocation> _location;
        sg_size_t _ram_footprint;
        std::shared_ptr<DataFile> _ram_file;
    };
} // namespace wrench

#endif // WRENCH_IMAGE_H

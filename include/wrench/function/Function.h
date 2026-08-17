/**
* Copyright (c) 2025. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FUNCTION_H
#define WRENCH_FUNCTION_H

#include <string>
#include <functional>
#include <memory>

namespace wrench {

    class Image;
    class FunctionInput;
    class FunctionOutput;
    class ServerlessComputeService;
    class StorageService;
    class FileLocation;
    class DataFile;

    /**
     * @brief A class that implements the notion of a function that
     *        can be invoked at a serverless compute service
     */
    class Function {
    public:

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        Function(const std::string &name,
                 const std::function<std::shared_ptr<FunctionOutput>(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> &lambda,
                 const std::shared_ptr<Image> &image);

        [[nodiscard]] std::shared_ptr<FunctionOutput> execute(const std::shared_ptr<FunctionInput> &input, const std::shared_ptr<StorageService> &storage_service) const;

        /***********************/
        /** \endcond           */
        /***********************/

        [[nodiscard]] std::string getName() const;
        [[nodiscard]] std::shared_ptr<Image> getImage() const;
        [[nodiscard]] std::shared_ptr<DataFile> getImageFile() const;


    private:
        friend class FunctionManager;

        std::string _name; // the name of the function
        std::function<std::shared_ptr<FunctionOutput>(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> _lambda; // the function logic
        std::shared_ptr<Image> _image;
    };

} // namespace wrench

#endif // WRENCH_FUNCTION_H

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
#include "wrench/services/storage/storage_helpers/FileLocation.h"
#include "wrench/managers/function_manager/FunctionInput.h"
#include "wrench/managers/function_manager/FunctionOutput.h"

namespace wrench {

    class ServerlessComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief 
     */
    class Function {
    public:

        Function(const std::string &name,
                 const std::function<std::shared_ptr<FunctionOutput>(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> &lambda,
                 const std::shared_ptr<FileLocation> &image);

        [[nodiscard]] std::string getName() const;

        [[nodiscard]] std::shared_ptr<FileLocation> getImage() const;

        [[nodiscard]] std::shared_ptr<FunctionOutput> execute(const std::shared_ptr<FunctionInput> &input, const std::shared_ptr<StorageService> &storage_service) const;

    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;

        std::string _name; // the name of the function
        std::function<std::shared_ptr<FunctionOutput>(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> _lambda; // the function logic
        std::shared_ptr<FileLocation> _image; // the file location of the function's container image
    };
    
    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_FUNCTION_H

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

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    class ComputeService;

    /**
     * @brief Represents a serverless function, encapsulating its metadata and behavior.
     */
    class Function {
    public:
        /**
         * @brief Constructs a Function object.
         * @param name The name of the function.
         * @param lambda The function logic implemented as a lambda.
         * @param image The file location of the function's container image.
         * @param code The file location of the function's code.
         */
        Function(const std::string &name,
                 const std::function<std::string(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> &lambda,
                 const std::shared_ptr<FileLocation> &image,
                 const std::shared_ptr<FileLocation> &code)
            : _name(name), _lambda(lambda), _image(image), _code(code) {}

        /**
         * @brief Gets the name of the function.
         * @return The name of the function.
         */
        std::string getName() const { return _name; }

        /**
         * @brief Executes the function with the provided input and storage service.
         * @param input The input string for the function.
         * @param storage_service A shared pointer to a StorageService instance.
         * @return The result of the function execution.
         */
        std::string execute(const std::shared_ptr<FunctionInput> &input, const std::shared_ptr<StorageService> &storage_service) const {
            return _lambda(input, storage_service);
        }

    private:
        std::string _name; ///< The name of the function.
        std::function<std::string(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> _lambda; ///< The function logic.
        std::shared_ptr<FileLocation> _image; ///< The file location of the function's container image.
        std::shared_ptr<FileLocation> _code; ///< The file location of the function's code.
    };
    
    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_FUNCTION_H

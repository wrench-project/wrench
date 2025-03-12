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

    class ServerlessComputeService;

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief 
     */
    class Function {
    public:
        /**
         * @brief 
         * @param name 
         * @param lambda 
         * @param image 
         * @param code 
         */
        Function(const std::string &name,
                 const std::function<std::string(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> &lambda,
                 const std::shared_ptr<FileLocation> &image,
                 const std::shared_ptr<FileLocation> &code);

        /**
         * @brief 
         * @return 
         */
        std::string getName() const;

        /**
         * @brief 
         * @return 
         */
        std::shared_ptr<FileLocation> getImage() const;

        /**
         * @brief 
         * @param input 
         * @param storage_service 
         * @return 
         */
        std::string execute(const std::shared_ptr<FunctionInput> &input, const std::shared_ptr<StorageService> &storage_service) const;

    private:
        friend class FunctionManager;
        friend class ServerlessComputeService;

        std::string _name; // the name of the function
        std::function<std::string(const std::shared_ptr<FunctionInput> &, const std::shared_ptr<StorageService> &)> _lambda; // the function logic
        std::shared_ptr<FileLocation> _image; // the file location of the function's container image
        std::shared_ptr<FileLocation> _code; // the file location of the function's code
    };
    
    /***********************/
    /** \endcond           */
    /***********************/

} // namespace wrench

#endif // WRENCH_FUNCTION_H

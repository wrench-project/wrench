/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <wrench-dev.h>

namespace wrench {

    /**
     *  @brief An Execution Controller implementation
     */
    class Controller : public ExecutionController {

    public:
        // Constructor
        Controller(
                const std::shared_ptr<BareMetalComputeService> &bare_metal_compute_service,
                const std::shared_ptr<SimpleStorageService> &storage_service,
                const std::string &hostname);

    protected:
        // Overridden method
        void processEventCompoundJobCompletion(const std::shared_ptr<CompoundJobCompletedEvent>&) override;

    private:
        // main() method of the Execution Controller
        int main() override;

        const std::shared_ptr<BareMetalComputeService> bare_metal_compute_service;
        const std::shared_ptr<SimpleStorageService> storage_service;
    };
}// namespace wrench
#endif//CONTROLLER_H

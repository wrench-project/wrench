
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

#include <wrench/services/storage/xrootd/XRootD.h>
#include <wrench-dev.h>

namespace wrench {

    /**
     *  @brief A Workflow Management System (WMS) implementation
     */
    class Controller : public ExecutionController {

    public:
        // Constructor
        Controller(
				const std::shared_ptr<BareMetalComputeService> bare_metal_compute_service,
                const std::shared_ptr<wrench::XRootD::Node> &storage_service,
				XRootD::XRootD *xrootdManager,
                const std::string &hostname);

    protected:
        // Overridden method
        void processEventCompoundJobCompletion(std::shared_ptr<CompoundJobCompletedEvent>) override;

    private:
        // main() method of the WMS
        int main() override;

        const std::shared_ptr<BareMetalComputeService> bare_metal_compute_service;
        const std::shared_ptr<wrench::XRootD::Node> root;
		XRootD::XRootD *xrootdManager;
    };
}// namespace wrench
#endif//CONTROLLER_H

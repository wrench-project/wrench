/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_COPY_ACTION_H
#define WRENCH_FILE_COPY_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class WorkflowFile;
    class FileLocation;

    class FileCopyAction : public Action {

    public:
        std::shared_ptr<WorkflowFile> getFile() const;
        std::shared_ptr<FileLocation> getSourceFileLocation() const;
        std::shared_ptr<FileLocation> getDestinationFileLocation() const;

    protected:
        friend class CompoundJob;

        FileCopyAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                std::shared_ptr<WorkflowFile> file,
                                std::shared_ptr<FileLocation> src_file_location,
                                std::shared_ptr<FileLocation> dst_file_location);


        void execute(std::shared_ptr<ActionExecutor> action_executor,unsigned long num_threads, double ram_footprint) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;

    private:
        std::shared_ptr<WorkflowFile> file;
        std::shared_ptr<FileLocation> src_file_location;
        std::shared_ptr<FileLocation> dst_file_location;

    };
}

#endif //WRENCH_FILE_COPY_ACTION_H

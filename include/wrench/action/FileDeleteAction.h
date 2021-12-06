/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_FILE_DELETE_ACTION_H
#define WRENCH_FILE_DELETE_ACTION_H

#include <string>

#include "wrench/action/Action.h"

namespace wrench {

    class WorkflowFile;
    class FileLocation;

    /**
     * @brief A class that implements a file deletion action
     */
    class FileDeleteAction : public Action {

    public:
        WorkflowFile *getFile() const;
        std::shared_ptr<FileLocation> getFileLocation() const;
        bool usesScratch() const override;

    protected:
        friend class CompoundJob;

        FileDeleteAction(const std::string& name, std::shared_ptr<CompoundJob> job,
                                WorkflowFile *file, std::shared_ptr<FileLocation> file_location);


        void execute(std::shared_ptr<ActionExecutor> action_executor) override;
        void terminate(std::shared_ptr<ActionExecutor> action_executor) override;

    private:
        WorkflowFile *file;
        std::shared_ptr<FileLocation> file_location;

    };
}

#endif //WRENCH_FILE_DELETE_ACTION_H

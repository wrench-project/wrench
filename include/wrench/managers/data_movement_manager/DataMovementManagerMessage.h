/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DATAMOVEMENTMANAGERMESSAGE_H
#define WRENCH_DATAMOVEMENTMANAGERMESSAGE_H

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/services/compute/ComputeService.h"
#include "wrench/job/StandardJob.h"
#include "wrench-dev.h"

namespace wrench {


    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief Top-level class for messages received/sent by a DataMovementManager
     */
    class DataMovementManagerMessage : public SimulationMessage {
    protected:
        explicit DataMovementManagerMessage();
    };

    /**
   * @brief A message sent by a DataMovementManager upon file copy completion
   */
    class DataManagerFileCopyAnswerMessage : public DataMovementManagerMessage {
    public:
        DataManagerFileCopyAnswerMessage(std::shared_ptr<FileLocation> src_location,
                                         std::shared_ptr<FileLocation> dst_location,
                                         bool success,
                                         std::shared_ptr<FailureCause> failure_cause);
        /** @brief The src location */
        std::shared_ptr<FileLocation> src_location;
        /** @brief The dst location */
        std::shared_ptr<FileLocation> dst_location;
        /** @brief Whether the operation succeeded */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
   * @brief A message sent by a DataMovementManager upon file read completion
   */
    class DataManagerFileReadAnswerMessage : public DataMovementManagerMessage {
    public:
        DataManagerFileReadAnswerMessage(std::shared_ptr<FileLocation> location,
                                         double num_bytes,
                                         bool success,
                                         std::shared_ptr<FailureCause> failure_cause);
        /** @brief The read location */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes to read */
        double num_bytes;
        /** @brief Whether the operation succeeded */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
   * @brief A message sent by a DataMovementManager upon file write completion
   */
    class DataManagerFileWriteAnswerMessage : public DataMovementManagerMessage {
    public:
        DataManagerFileWriteAnswerMessage(std::shared_ptr<FileLocation> location,
                                          bool success,
                                          std::shared_ptr<FailureCause> failure_cause);
        /** @brief The write location */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the operation succeeded */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> failure_cause;
    };


    /**
   * @brief A message sent to a DataMovementManager from a FileReaderThread
   */
    class DataMovementManagerFileReaderThreadMessage : public DataMovementManagerMessage {
    public:
        DataMovementManagerFileReaderThreadMessage(std::shared_ptr<FileLocation> location, double num_bytes,
                                                   bool success, std::shared_ptr<FailureCause> failure_cause);
        /** @brief The read location */
        std::shared_ptr<FileLocation> location;
        /** @brief The number of bytes to read */
        double num_bytes;
        /** @brief Whether the operation succeeded */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /**
   * @brief A message sent to a DataMovementManager from a FileReaderThread
   */
    class DataMovementManagerFileWriterThreadMessage : public DataMovementManagerMessage {
    public:
        DataMovementManagerFileWriterThreadMessage(std::shared_ptr<FileLocation> location, bool success, std::shared_ptr<FailureCause> failure_cause);
        /** @brief The write location */
        std::shared_ptr<FileLocation> location;
        /** @brief Whether the operation succeeded */
        bool success;
        /** @brief The failure cause, if any */
        std::shared_ptr<FailureCause> failure_cause;
    };

    /***********************/
    /** \endcond           */
    /***********************/

}// namespace wrench

#endif//WRENCH_DATAMOVEMENTMANAGERMESSAGE_H

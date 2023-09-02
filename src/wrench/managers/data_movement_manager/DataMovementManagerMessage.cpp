/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "wrench/managers/data_movement_manager/DataMovementManagerMessage.h"

#include <utility>

namespace wrench {

    /**
     * @brief Constructor
     */
    DataMovementManagerMessage::DataMovementManagerMessage() : SimulationMessage(0) {
    }


    /**
     * @brief Constructor
     * @param src_location: the source location
     * @param dst_location: the destination location
     * @param success: true if success, false otherwise
     * @param failure_cause: a failure cause (nullptr is success if false)
     */
    DataManagerFileCopyAnswerMessage::DataManagerFileCopyAnswerMessage(std::shared_ptr<FileLocation> src_location,
                                                                       std::shared_ptr<FileLocation> dst_location,
                                                                       bool success,
                                                                       std::shared_ptr<FailureCause> failure_cause) : DataMovementManagerMessage(),
                                                                                                                      src_location(std::move(src_location)),
                                                                                                                      dst_location(std::move(dst_location)), success(success), failure_cause(std::move(failure_cause)) {
    }

    /**
     * @brief Constructor
     * @param location: a location
     * @param num_bytes: the number of bytes for the read
     * @param success: true if success, false otherwise
     * @param failure_cause: a failure cause (nullptr is success if false)
     */
    DataManagerFileReadAnswerMessage::DataManagerFileReadAnswerMessage(std::shared_ptr<FileLocation> location,
                                                                       double num_bytes,
                                                                       bool success,
                                                                       std::shared_ptr<FailureCause> failure_cause) : DataMovementManagerMessage(),
                                                                                                                      location(std::move(location)), num_bytes(num_bytes), success(success), failure_cause(std::move(failure_cause)) {
    }

    /**
     * @brief Constructor
     * @param location: a location
     * @param success: true if success, false otherwise
     * @param failure_cause: a failure cause (nullptr is success if false)
     */
    DataManagerFileWriteAnswerMessage::DataManagerFileWriteAnswerMessage(std::shared_ptr<FileLocation> location,
                                                                         bool success,
                                                                         std::shared_ptr<FailureCause> failure_cause) : DataMovementManagerMessage(),
                                                                                                                        location(std::move(location)), success(success), failure_cause(std::move(failure_cause)) {
    }


    /**
     * @brief Constructor
     * @param location: a location
     * @param num_bytes: a number of bytes to read
     * @param success: true if success, false otherwise
     * @param failure_cause: a failure cause (nullptr is success if false)
     */
    DataMovementManagerFileReaderThreadMessage::DataMovementManagerFileReaderThreadMessage(std::shared_ptr<FileLocation> location,
                                                                                           double num_bytes,
                                                                                           bool success,
                                                                                           std::shared_ptr<FailureCause> failure_cause)
        : DataMovementManagerMessage(), location(std::move(location)), num_bytes(num_bytes), success(success), failure_cause(std::move(failure_cause)) {
    }

    /**
     * @brief Constructor
     * @param location: a location
     * @param success: true if success, false otherwise
     * @param failure_cause: a failure cause (nullptr is success if false)
     */
    DataMovementManagerFileWriterThreadMessage::DataMovementManagerFileWriterThreadMessage(std::shared_ptr<FileLocation> location,
                                                                                           bool success,
                                                                                           std::shared_ptr<FailureCause> failure_cause)
        : DataMovementManagerMessage(), location(std::move(location)), success(success), failure_cause(std::move(failure_cause)) {
    }


}// namespace wrench

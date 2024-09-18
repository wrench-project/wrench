/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <utility>
#include <wrench/logging/TerminalOutput.h>

#include <wrench/data_file/DataFile.h>

WRENCH_LOG_CATEGORY(wrench_core_data_file, "Log category for DataFile");

namespace wrench {

/**
 * @brief Destructor
 */
DataFile::~DataFile() { UNTRACK_OBJECT("file"); }

/**
 * @brief Constructor
 *
 * @param id: the file id
 * @param s: the file size
 */
DataFile::DataFile(std::string id, double s) : id(std::move(id)), size(s) {
  TRACK_OBJECT("file");
}

/**
 * @brief Get the file size
 * @return a size in bytes
 */
double DataFile::getSize() const { return this->size; }

/**
 * @brief Set the file size (to be used only in very specific
 * cases in which it is guaranteed that changing a file's size after that file
 * has been created is a valid thing to do)
 * @param s: a size in byte
 */
void DataFile::setSize(double s) { this->size = s; }

/**
 * @brief Get the file id
 * @return the id
 */
std::string DataFile::getID() const { return this->id; }

} // namespace wrench

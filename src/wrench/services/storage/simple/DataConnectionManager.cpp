/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "DataConnectionManager.h"

namespace wrench {

    DataConnectionManager::DataConnectionManager(unsigned long num_connections) {
      this->num_connections = num_connections;

    }
};
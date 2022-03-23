/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_UNIQUEPLATFORMFILEPATH_H
#define WRENCH_UNIQUEPLATFORMFILEPATH_H

#include <unistd.h>

// Convenient macro to generate a platform file path unique to every user
#define UNIQUE_TMP_PATH_PREFIX ("/tmp/" + std::to_string(getuid()) + "_")
#define UNIQUE_PREFIX (std::to_string(getuid()) + "_")

#endif//WRENCH_UNIQUEPLATFORMFILEPATH_H

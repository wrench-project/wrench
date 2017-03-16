/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief TBD
 */

#include "Pilotjob.h"

namespace wrench {

		Pilotjob::Pilotjob() {
			this->state = SUBMITTED;
			this->compute_service = nullptr;
		}

		Pilotjob::State Pilotjob::getState() {
			return this->state;
		}

		ComputeService *Pilotjob::getComputeService() {
			return this->compute_service;
		}

};
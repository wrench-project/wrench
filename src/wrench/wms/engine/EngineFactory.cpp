/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::EngineFactory is a factory class for WMS engines
 */

#include "wms/engine/EngineFactory.h"

namespace wrench {

	EngineFactory *EngineFactory::getInstance() {
		static EngineFactory fact;
		return &fact;
	}

	uint16_t EngineFactory::Register(uint16_t wms_id, t_pfFactory factoryMethod) {
		s_list[wms_id] = factoryMethod;
		return wms_id;
	}

	std::unique_ptr<WMS> EngineFactory::Create(uint16_t wms_id) {
		return s_list[wms_id]();
	}

	EngineFactory::EngineFactory() {}
}
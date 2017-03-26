/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief wrench::SimpleWMS implements a simple WMS abstraction
 */

#ifndef WRENCH_SIMPLEWMS_H
#define WRENCH_SIMPLEWMS_H

#include "wms/engine/EngineFactory.h"
#include "wms/engine/simple_wms/SimpleWMSDaemon.h"

namespace wrench {

	const char wms_name[] = "SimpleWMS";

	class SimpleWMS : public EngineTmpl<wms_name, SimpleWMS, SimpleWMSDaemon> {

	public:
		SimpleWMS();
	};
}
#endif //WRENCH_SIMPLEWMS_H

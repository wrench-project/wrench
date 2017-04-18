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

#ifndef WRENCH_ENGINEFACTORY_H
#define WRENCH_ENGINEFACTORY_H

#include <map>
#include "wms/engine/EngineTmpl.h"

namespace wrench {

	class EngineFactory {

	public:
		typedef std::unique_ptr<WMS> (*t_pfFactory)();

		static EngineFactory *getInstance();
		std::string Register(std::string wms_id, t_pfFactory factory_method);
		std::unique_ptr<WMS> Create(std::string wms_id);

		std::map<std::string, t_pfFactory> s_list;

	private:
		EngineFactory();
	};

	template<const char *TYPE, typename IMPL>
	const std::string EngineTmpl<TYPE, IMPL>::WMS_ID = EngineFactory::getInstance()->Register(
			EngineTmpl<TYPE, IMPL>::_WMS_ID, &EngineTmpl<TYPE, IMPL>::Create);
}

#endif //WRENCH_ENGINEFACTORY_H

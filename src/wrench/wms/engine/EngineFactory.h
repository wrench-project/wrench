/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_ENGINEFACTORY_H
#define WRENCH_ENGINEFACTORY_H

#include <map>
#include "wms/engine/EngineTmpl.h"

namespace wrench {

		/***********************/
		/** \cond INTERNAL     */
		/***********************/

		/**
		 * @brief A factory class for WMS engines
		 */
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

		/***********************/
		/** \cond INTERNAL     */
		/***********************/
}

#endif //WRENCH_ENGINEFACTORY_H

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * @brief Simple macros for doing color
 */


#ifndef WRENCH_S4U_COLORLOGGING_H
#define WRENCH_S4U_COLORLOGGING_H


#include <simgrid/s4u/forward.hpp>
#include <map>

namespace wrench {

		/** Define color cores **/
		#define WRENCH_LOGGING_COLOR_RED			"\033[1;31m"
		#define WRENCH_LOGGING_COLOR_GREEN		"\033[1;32m"
		#define WRENCH_LOGGING_COLOR_YELLOW		"\033[1;33m"
		#define WRENCH_LOGGING_COLOR_BLUE			"\033[1;34m"
		#define WRENCH_LOGGING_COLOR_MAGENTA	"\033[1;35m"
		#define WRENCH_LOGGING_COLOR_CYAN			"\033[1;36m"

		/**	Wrapper macros around XBT_INFO **/
		#define WRENCH_INFO(...)  ColorLogging::beginThisProcessColor(); XBT_INFO(__VA_ARGS__) ; ColorLogging::endThisProcessColor()
		#define WRENCH_DEBUG(...)  ColorLogging::beginThisProcessColor(); XBT_DEBUG(__VA_ARGS__) ; ColorLogging::endThisProcessColor()

		class ColorLogging {

		public:
				static void beginThisProcessColor();
				static void endThisProcessColor();
				static void setThisProcessLoggingColor(std::string color);

		private:
				static std::map<simgrid::s4u::ActorPtr , std::string> colormap;

				static std::string getThisProcessLoggingColor();




		};
};


#endif //WRENCH_S4U_COLORLOGGING_H

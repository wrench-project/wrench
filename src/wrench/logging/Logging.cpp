/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string>
#include <simgrid/s4u/Actor.hpp>
#include <iostream>
#include "Logging.h"

namespace wrench {

		std::map<simgrid::s4u::ActorPtr, std::string> Logging::colormap;

		/**
		 * @brief Set the color of log messages printed to the terminal
		 *
		 * @param color is WRENCH_LOGGING_COLOR_RED, WRENCH_LOGGING_COLOR_GREEN, etc.
		 * @return void
		 */
		void Logging::setThisProcessLoggingColor(std::string color) {
			Logging::colormap[simgrid::s4u::Actor::self()] = color;
		}


		/**
		 * @brief Turn on colored output for the current process
		 * @return void
		 */
		void Logging::beginThisProcessColor() {
			// Comment this line out to do no colors
			std::cerr << Logging::getThisProcessLoggingColor();
		}

		/**
		 * @brief Turn off colored output for the current process
		 * @return void
		 */
		void Logging::endThisProcessColor() {
			std::cerr << "\033[0m";
		}


		/**
		 * @brief Get the current output color for the current process
		 * @return the color as a string
		 */
		std::string Logging::getThisProcessLoggingColor() {
			if (Logging::colormap.find(simgrid::s4u::Actor::self()) != Logging::colormap.end()) {
				return Logging::colormap[simgrid::s4u::Actor::self()];
			} else {
				return "";
			}
		}

};

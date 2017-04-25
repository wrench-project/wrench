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
#include "TerminalOutput.h"

namespace wrench {

    std::map<simgrid::s4u::ActorPtr, std::string> TerminalOutput::colormap;
    bool TerminalOutput::color_enabled = true;

    /**
     * @brief Set the color of log messages printed to the terminal
     *
     * @param color: WRENCH_LOGGING_COLOR_RED, WRENCH_LOGGING_COLOR_GREEN, etc.
     */
    void TerminalOutput::setThisProcessLoggingColor(std::string color) {
      TerminalOutput::colormap[simgrid::s4u::Actor::self()] = color;
    }


    /**
     * @brief Turn on colored output for the current process
     */
    void TerminalOutput::beginThisProcessColor() {
      if (TerminalOutput::color_enabled) {
        std::cerr << TerminalOutput::getThisProcessLoggingColor();
      }
    }

    /**
     * @brief Turn off colored output for the current process
     */
    void TerminalOutput::endThisProcessColor() {
      if (TerminalOutput::color_enabled) {
        std::cerr << "\033[0m";
      }
    }

    /**
     * @brief Disable color terminal output
     */
    void TerminalOutput::disableColor() {
      TerminalOutput::color_enabled = false;
    }

    /**
     * @brief Get the current output color for the current process
     * @return the color as a string
     */
    std::string TerminalOutput::getThisProcessLoggingColor() {
      if (TerminalOutput::colormap.find(simgrid::s4u::Actor::self()) != TerminalOutput::colormap.end()) {
        return TerminalOutput::colormap[simgrid::s4u::Actor::self()];
      } else {
        return "";
      }
    }

};

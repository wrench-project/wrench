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
#include "wrench/logging/TerminalOutput.h"

namespace wrench {

    const char *TerminalOutput::color_codes[] = {
            "\033[1;30m",
            "\033[1;31m",
            "\033[1;32m",
            "\033[1;33m",
            "\033[1;34m",
            "\033[1;35m",
            "\033[1;36m",
            "\033[1;37m",
    };

    std::map<simgrid::s4u::Actor *, std::string> TerminalOutput::colormap;
    bool TerminalOutput::color_enabled = true;

    /**
     * @brief Set the color of log messages printed to the terminal
     *
     * @param color: a terminal output color
     */
    void TerminalOutput::setThisProcessLoggingColor(Color color) {
        TerminalOutput::colormap[simgrid::s4u::Actor::self()] = TerminalOutput::color_codes[color];
    }

    /**
     * @brief Turn on colored output for the calling process
     */
    void TerminalOutput::beginThisProcessColor() {

        if (TerminalOutput::color_enabled) {
            std::cerr << TerminalOutput::getThisProcessLoggingColor();
        }
    }

    /**
     * @brief Turn off colored output for the calling process
     */
    void TerminalOutput::endThisProcessColor() {
        if (TerminalOutput::color_enabled) {
            std::cerr << "\033[0m";
        }
    }

    /**
     * @brief Disable color terminal output for all processes
     */
    void TerminalOutput::disableColor() {
        TerminalOutput::color_enabled = false;
    }

    /**
     * @brief Get the current output color ASCII code sequence for the current process
     * @return the color ASCII code sequence as a string
     */
    std::string TerminalOutput::getThisProcessLoggingColor() {

        if (simgrid::s4u::this_actor::is_maestro() ||
            (TerminalOutput::colormap.find(simgrid::s4u::Actor::self()) == TerminalOutput::colormap.end())) {
            return "";
        } else {
            return TerminalOutput::colormap[simgrid::s4u::Actor::self()];
        }
    }

};

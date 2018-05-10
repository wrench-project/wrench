/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_S4U_COLORLOGGING_H
#define WRENCH_S4U_COLORLOGGING_H

#include <map>

#include <simgrid/s4u/Actor.hpp>

#include <iostream>

namespace wrench {

/* Defined color codes */
#define COLOR_BLACK      "\033[1;30m"
#define COLOR_RED      "\033[1;31m"
#define COLOR_GREEN    "\033[1;32m"
#define COLOR_YELLOW    "\033[1;33m"
#define COLOR_BLUE      "\033[1;34m"
#define COLOR_MAGENTA  "\033[1;35m"
#define COLOR_CYAN      "\033[1;36m"
#define COLOR_WHITE      "\033[1;37m"

/* Wrappers around XBT_* macros */

#define WRENCH_INFO(...)  wrench::TerminalOutput::beginThisProcessColor(); XBT_INFO(__VA_ARGS__) ; wrench::TerminalOutput::endThisProcessColor()

#define WRENCH_DEBUG(...)  wrench::TerminalOutput::beginThisProcessColor(); XBT_DEBUG(__VA_ARGS__) ; wrench::TerminalOutput::endThisProcessColor()

#define WRENCH_WARN(...)  wrench::TerminalOutput::beginThisProcessColor(); XBT_WARN(__VA_ARGS__) ; wrench::TerminalOutput::endThisProcessColor()

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Color-enabling wrappers around Simgrid's XBT_INFO, XBT_DEBUG, XBT_WARN logging macros
     */
    class TerminalOutput {

    public:

        static void setThisProcessLoggingColor(std::string color);

        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        static void beginThisProcessColor();

        static void endThisProcessColor();

        static void disableColor();

        /***********************/
        /** \endcond           */
        /***********************/

    private:
        static std::map<simgrid::s4u::ActorPtr, std::string> colormap;

        static std::string getThisProcessLoggingColor();

        static bool color_enabled;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_S4U_COLORLOGGING_H

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

#include <simgrid/s4u/forward.hpp>
#include <map>

/* Defined color codes */
#define WRENCH_LOGGING_COLOR_RED      "\033[1;31m"
#define WRENCH_LOGGING_COLOR_GREEN    "\033[1;32m"
#define WRENCH_LOGGING_COLOR_YELLOW    "\033[1;33m"
#define WRENCH_LOGGING_COLOR_BLUE      "\033[1;34m"
#define WRENCH_LOGGING_COLOR_MAGENTA  "\033[1;35m"
#define WRENCH_LOGGING_COLOR_CYAN      "\033[1;36m"

/* Wrappers around XBT_* macros */
#define WRENCH_INFO(...)  TerminalOutput::beginThisProcessColor(); XBT_INFO(__VA_ARGS__) ; TerminalOutput::endThisProcessColor()
#define WRENCH_DEBUG(...)  TerminalOutput::beginThisProcessColor(); XBT_DEBUG(__VA_ARGS__) ; TerminalOutput::endThisProcessColor()
#define WRENCH_WARN(...)  TerminalOutput::beginThisProcessColor(); XBT_WARN(__VA_ARGS__) ; TerminalOutput::endThisProcessColor()

namespace wrench {

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Color-enabling wrappers around Simgrid's logging macros.
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
        /** \cond INTERNAL    */
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

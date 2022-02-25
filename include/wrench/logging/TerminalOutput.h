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
#include <xbt/log.h>

#include <iostream>

namespace wrench {


/* Wrappers around XBT_* macros, using a bit of those macro's internal magic as well
 * to avoid generating useless (but space consuming) color ASCII codes
 */

#define WRENCH_LOG_CATEGORY(cname, desc) XBT_LOG_NEW_DEFAULT_CATEGORY(cname, desc)

#define WRENCH_INFO(...);  \
    if (_XBT_LOG_ISENABLEDV((*_simgrid_log_category__default), xbt_log_priority_info)) { \
      wrench::TerminalOutput::beginThisProcessColor();  \
      XBT_INFO(__VA_ARGS__) ;  \
      wrench::TerminalOutput::endThisProcessColor(); \
    }

#define WRENCH_DEBUG(...);  \
    if (_XBT_LOG_ISENABLEDV((*_simgrid_log_category__default), xbt_log_priority_debug)) { \
      wrench::TerminalOutput::beginThisProcessColor();  \
      XBT_DEBUG(__VA_ARGS__) ;  \
      wrench::TerminalOutput::endThisProcessColor(); \
    }

#define WRENCH_WARN(...);  \
    if (_XBT_LOG_ISENABLEDV((*_simgrid_log_category__default), xbt_log_priority_warning)) { \
      wrench::TerminalOutput::beginThisProcessColor();  \
      XBT_WARN(__VA_ARGS__) ;  \
      wrench::TerminalOutput::endThisProcessColor(); \
    }

    /***********************/
    /** \cond DEVELOPER    */
    /***********************/

    /**
     * @brief Color-enabling wrappers around Simgrid's XBT_INFO, XBT_DEBUG, XBT_WARN logging macros
     */
    class TerminalOutput {

    public:

        /** @brief Terminal output color enum */
        enum Color {
            /** @brief Black text **/
                    COLOR_BLACK,
            /** @brief Red text **/
                    COLOR_RED,
            /** @brief Green text **/
                    COLOR_GREEN,
            /** @brief Yellow text **/
                    COLOR_YELLOW,
            /** @brief Blue text **/
                    COLOR_BLUE,
            /** @brief Magenta text **/
                    COLOR_MAGENTA,
            /** @brief Cyan text **/
                    COLOR_CYAN,
            /** @brief White text **/
                    COLOR_WHITE,
        };

        static void setThisProcessLoggingColor(Color color);



        /***********************/
        /** \cond INTERNAL     */
        /***********************/

        static void beginThisProcessColor();

        static void endThisProcessColor();

        static void disableColor();

//        static void disableLog();

        /***********************/
        /** \endcond           */
        /***********************/

    private:

        static const char * color_codes[];

        static std::unordered_map<simgrid::s4u::Actor *, std::string> colormap;

        static std::string getThisProcessLoggingColor();

        static bool color_enabled;

    };

    /***********************/
    /** \endcond           */
    /***********************/
};


#endif //WRENCH_S4U_COLORLOGGING_H

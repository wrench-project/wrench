/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_UNITPARSER_H
#define WRENCH_UNITPARSER_H

#include <unordered_map>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    class unit_scale;

    /**
     * @brief A class used to part string specification of values with units into
     *        a single value (e.g., "2KB" -> 2048 bytes, "2Kb" -> 256 bytes)
     */
    class UnitParser {

        /**
         * @brief A helper nested class to facilitate unit conversion
         * (Essentially Cut-And-Pasted from simgrid/src/surf/xml/surfxml_sax_cb.cpp)
         */
        class unit_scale : public std::unordered_map<std::string, double> {
        public:
            using std::unordered_map<std::string, double>::unordered_map;
            // tuples are : <unit, value for unit, base (2 or 10), true if abbreviated>
            explicit unit_scale(std::initializer_list<std::tuple<const std::string, double, int, bool>> generators);
        };

    private:
        static double parseValueWithUnit(const std::string &string, const unit_scale &units, const char *default_unit);

    public:
        static double parse_size(const std::string &string);
        static double parse_compute_speed(const std::string &string);
        static double parse_bandwidth(const std::string &string);
        static double parse_time(const std::string &string);

    };

    /***********************/
    /** \endcond           */
    /***********************/
};// namespace wrench


#endif//WRENCH_UNITPARSER_H

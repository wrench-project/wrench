/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cerrno>
#include <stdlib.h>
#include <wrench/util/UnitParser.h>

namespace wrench {


    //    /**
    //     * @brief A helper nested class to facilitate unit conversion
    //     * (Essentially Cut-And-Pasted from simgrid/src/surf/xml/surfxml_sax_cb.cpp)
    //     */
    //    class unit_scale : public WRENCH_MESSAGE_PAYLOADCOLLECTION_TYPE {
    //    public:
    //        using std::unordered_map<std::string, double>::unordered_map;
    //        // tuples are : <unit, value for unit, base (2 or 10), true if abbreviated>
    //        explicit unit_scale(std::initializer_list<std::tuple<const std::string, double, int, bool>> generators);
    //    };

    /**
     * @brief Constructor
     * @param generators: generators
     */
    UnitParser::unit_scale::unit_scale(
            std::initializer_list<std::tuple<const std::string, double, int, bool>> generators) {
        for (const auto &gen: generators) {
            const std::string &unit = std::get<0>(gen);
            double value = std::get<1>(gen);
            const int base = std::get<2>(gen);
            const bool abbrev = std::get<3>(gen);
            double mult;
            std::vector<std::string> prefixes;
            switch (base) {
                case 2:
                    mult = 1024.0;
                    prefixes = abbrev ? std::vector<std::string>{"Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi"}
                                      : std::vector<std::string>{"kibi", "mebi", "gibi", "tebi", "pebi", "exbi", "zebi",
                                                                 "yobi"};
                    break;
                case 10:
                    mult = 1000.0;
                    prefixes = abbrev ? std::vector<std::string>{"k", "M", "G", "T", "P", "E", "Z", "Y"}
                                      : std::vector<std::string>{"kilo", "mega", "giga", "tera", "peta", "exa", "zeta",
                                                                 "yotta"};
                    break;
                default:
                    throw std::runtime_error("UnitParser::unit_scale::unit_scale(): The impossible has happened");
            }
            emplace(unit, value);
            for (const auto &prefix: prefixes) {
                value *= mult;
                emplace(prefix + unit, value);
            }
        }
    }

    double
    UnitParser::parseValueWithUnit(const std::string &string, const UnitParser::unit_scale &units, const char *default_unit) {
        char *ptr;
        const char *c_string = string.c_str();
        errno = 0;

        double res = strtod(c_string, &ptr);
        if (errno == ERANGE)
            throw std::runtime_error("Value out of range when parsing value " + string);
        if (ptr == string)
            throw std::runtime_error("Cannot parse value " + string);
        if (ptr[0] == '\0') {
            if (res == 0)
                return res;// Ok, 0 can be unit-less
            ptr = (char *) default_unit;
        }
        auto u = units.find(ptr);
        if (u == units.end())
            throw std::runtime_error("Unknown unit '" + std::string(ptr) + " when parsing value " + string);
        return res * u->second;
    }


    /**
     * @brief Given a string size specification with units (e.g., "13Mb") return the size in bytes
     * @param string: the size specification
     * @return the size in bytes
     *
     * @throws std::invalid_argument
     */
    double UnitParser::parse_size(const std::string &string) {
        static const UnitParser::unit_scale units{std::make_tuple("b", 0.125, 2, true),
                                                  std::make_tuple("b", 0.125, 10, true),
                                                  std::make_tuple("B", 1.0, 2, true),
                                                  std::make_tuple("B", 1.0, 10, true)};
        try {
            return parseValueWithUnit(string, units, "B");// default: bytes
        } catch (std::runtime_error &e) {
            throw std::invalid_argument(e.what());
        }
    }


    /**
     * @brief Given a string compute speed specification with units (e.g., "3f", "10Gf") return the size in bytes
     * @param string: the size specification
     * @return the size in bytes
     *
     * @throws std::invalid_argument
     */
    double UnitParser::parse_compute_speed(const std::string &string) {
        static const UnitParser::unit_scale units{std::make_tuple("f", 1.0, 10, true)};
        try {
            return parseValueWithUnit(string, units, "f");// default: bytes
        } catch (std::runtime_error &e) {
            throw std::invalid_argument(e.what());
        }
    }

    /**
    * @brief Given a string bandwidth specification with units (e.g., "3Mbps", "40MiBps") return the size in bytes
    * @param string: the bandwidth specification
    * @return the bandwidth in byte/sec
    *
    * @throws std::invalid_argument
    */
    double UnitParser::parse_bandwidth(const std::string &string) {
        static const UnitParser::unit_scale units{std::make_tuple("bps", 0.125, 2, true),
                                                  std::make_tuple("bps", 0.125, 10, true),
                                                  std::make_tuple("Bps", 1.0, 2, true),
                                                  std::make_tuple("Bps", 1.0, 10, true)};
        try {
            return parseValueWithUnit(string, units, "bps");// default: bits
        } catch (std::runtime_error &e) {
            throw std::invalid_argument(e.what());
        }
    }

    /**
    * @brief Given a string time with units (e.g., "10us", "10ms") return the time in seconds
    * @param string: the time specification
    * @return the time in second
    *
    * @throws std::invalid_argument
    */
    double UnitParser::parse_time(const std::string &string) {
        static const unit_scale units{std::make_pair("w", 7 * 24 * 60 * 60),
                                      std::make_pair("d", 24 * 60 * 60),
                                      std::make_pair("h", 60 * 60),
                                      std::make_pair("m", 60),
                                      std::make_pair("s", 1.0),
                                      std::make_pair("ms", 1e-3),
                                      std::make_pair("us", 1e-6),
                                      std::make_pair("ns", 1e-9),
                                      std::make_pair("ps", 1e-12)};
        try {
            return parseValueWithUnit(string, units, "s");// default: bits
        } catch (std::runtime_error &e) {
            throw std::invalid_argument(e.what());
        }
    }

};// namespace wrench

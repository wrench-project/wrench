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


namespace wrench {

    class unit_scale;

    class UnitParser {

    private:
        static double parseValueWithUnit(std::string string, const unit_scale &units, const char *default_unit);

    public:
        static double parse_size(std::string string);


    };
};


#endif //WRENCH_UNITPARSER_H

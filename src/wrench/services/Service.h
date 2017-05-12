/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICE_H
#define WRENCH_SERVICE_H


#include <string>
#include <map>

namespace wrench {

    class Service {

    protected:

        // Property stuff
        void setProperty(int, std::string);
        std::string getPropertyValueAsString(int);
        double getPropertyValueAsDouble(int);

    private:
        std::map<int, std::string> property_list;

    };

};


#endif //WRENCH_SERVICE_H

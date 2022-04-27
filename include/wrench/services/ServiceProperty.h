/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_SERVICEPROPERTY_H
#define WRENCH_SERVICEPROPERTY_H
#include <map>
#include <string>


namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/
    /**
     * @brief Property type
     */
    typedef int WRENCH_PROPERTY_TYPE;
    extern WRENCH_PROPERTY_TYPE WRENCH_PROPERTY_COUNT;
    /**
     * @brief Configurable properties for a Service
     */

    /***********************/
    /** \endcond           */
    /***********************/

    class ServiceProperty {

        static std::map<std::string,WRENCH_PROPERTY_TYPE> stringToPropertyMap;
        static std::map<WRENCH_PROPERTY_TYPE,std::string> propertyToStringMap;
    public:
        static WRENCH_PROPERTY_TYPE addServiceProperty(std::string classname,std::string messagePayload);
        static WRENCH_PROPERTY_TYPE translateString(std::string serviceProperty);
        static std::string translatePropertyType(WRENCH_PROPERTY_TYPE serviceProperty);

    };

};// namespace wrench
#define DECLARE_PROPERTY_NAME(name) static const wrench::WRENCH_PROPERTY_TYPE name
#define SET_PROPERTY_NAME(classname, name) const wrench::WRENCH_PROPERTY_TYPE classname::name = ++wrench::WRENCH_PROPERTY_COUNT
        //++wrench::WRENCH_PROPERTY_COUNT
        //#name

#endif//WRENCH_SERVICEPROPERTY_H

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceProperty.h>
#include <iostream>

namespace wrench {
    /**
     * @brief Service Property Count
     */
    WRENCH_PROPERTY_TYPE WRENCH_PROPERTY_COUNT = 0;

    /**
     * @brief Wrapper to ensure static map initialization happens before use
     * @return a reference to a map
     */
    std::map<std::string, WRENCH_PROPERTY_TYPE> &wrapper_stringToPropertyMap() {
        static std::map<std::string, WRENCH_PROPERTY_TYPE> stringToPropertyMap;
        return stringToPropertyMap;
    }

    /**
     * @brief Wrapper to ensure static map initialization happens before use
     * @return a reference to a map
     */
    std::map<WRENCH_PROPERTY_TYPE, std::string> &wrapper_propertyToStringMap() {
        static std::map<WRENCH_PROPERTY_TYPE, std::string> propertyToStringMap;
        return propertyToStringMap;
    }

    /**
     * @brief add new message to payload map.  DO NOT CALL THIS FUNCTION DIRECTLY, use SET_PROPERTY_NAME and DECLARE_PROPERTY_NAME
     * @param classname: The class to add the message too
     * @param serviceProperty: the name of the service property to add
     * @return a property type
     */
    WRENCH_PROPERTY_TYPE ServiceProperty::addServiceProperty(std::string classname, std::string serviceProperty) {
        ++WRENCH_PROPERTY_COUNT;
        wrapper_stringToPropertyMap()[classname + "::" + serviceProperty] = WRENCH_PROPERTY_COUNT;
        wrapper_propertyToStringMap()[WRENCH_PROPERTY_COUNT] = classname + "::" + serviceProperty;
        return WRENCH_PROPERTY_COUNT;
    }
    /**
     * @brief translate a string key to a property ID
     * @param serviceProperty: the name of the service Property to get in classname::serviceProperty form (Note: the classname must be the parent class that defines the property)
     * @return a property type
     */
    WRENCH_PROPERTY_TYPE ServiceProperty::translateString(std::string serviceProperty) {
        return wrapper_stringToPropertyMap().at(serviceProperty);
    }
    /**
     * @brief translate a property ID to a string key
     * @param serviceProperty: the ID of the service Property
     * @return a property type, as a string
     */
    std::string ServiceProperty::translatePropertyType(WRENCH_PROPERTY_TYPE serviceProperty) {
        return wrapper_propertyToStringMap().at(serviceProperty);
    }
};// namespace wrench

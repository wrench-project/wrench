/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <wrench/services/ServiceProperty.h>


namespace wrench {
    /**
     * @brief Service Property Count
     */
    WRENCH_PROPERTY_TYPE WRENCH_PROPERTY_COUNT = 0;
    /**
     * @brief Service Property Map used in string translation
     */
    std::map<std::string,WRENCH_PROPERTY_TYPE> ServiceProperty::stringToPropertyMap = {};
    std::map<WRENCH_PROPERTY_TYPE,std::string> ServiceProperty::propertyToStringMap = {};
    /**
     * @brief add new message to payload map.  DO NOT CALL THIS FUNCTION DIRECTLY, use SET_PROPERTY_NAME and DECLARE_PROPERTY_NAME
     * @param classname: The class to add the message too
     * @param serviceProperty: the name of the service property to add
     */
    WRENCH_PROPERTY_TYPE ServiceProperty::addServiceProperty(std::string classname,std::string serviceProperty){
        ++WRENCH_PROPERTY_COUNT;
        stringToPropertyMap[classname+"::"+serviceProperty]=WRENCH_PROPERTY_COUNT;
        propertyToStringMap[WRENCH_PROPERTY_COUNT]=classname+"::"+serviceProperty;
        //std::cout<<classname+"::"+messagePayload<<std::endl;
        return WRENCH_PROPERTY_COUNT;
    }
    /**
     * @brief translate a string key to a property ID
     * @param serviceProperty: the name of the service Property to get in classname::serviceProperty form (Note: the classname must be the parent class that defines the property)
     */
    WRENCH_PROPERTY_TYPE ServiceProperty::translateString(std::string serviceProperty) {
        return stringToPropertyMap[serviceProperty];
    }
    /**
     * @brief translate a property ID to a string key
     * @param serviceProperty: the ID of the service Property
     */
    std::string ServiceProperty::translatePropertyType(WRENCH_PROPERTY_TYPE serviceProperty) {
        return propertyToStringMap[serviceProperty];
    }
};// namespace wrench

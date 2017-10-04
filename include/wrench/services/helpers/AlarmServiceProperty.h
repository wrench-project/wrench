//
// Created by Suraj Pandey on 10/3/17.
//

#ifndef WRENCH_ALARMSERVICEPROPERTY_H
#define WRENCH_ALARMSERVICEPROPERTY_H


#include <wrench/services/ServiceProperty.h>

namespace wrench {

    class AlarmServiceProperty: public ServiceProperty {
    public:
        DECLARE_PROPERTY_NAME(ALARM_TIMEOUT_MESSAGE_PAYLOAD);
    };
}


#endif //WRENCH_ALARMSERVICEPROPERTY_H

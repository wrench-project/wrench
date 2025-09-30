/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_CUSTOM_EVENT_H
#define WRENCH_CUSTOM_EVENT_H

#include <string>
#include <utility>
#include "wrench/execution_events/ExecutionEvent.h"
#include "wrench/execution_controller/ExecutionControllerMessage.h"

/***********************/
/** \cond DEVELOPER    */
/***********************/

namespace wrench {

    /**
     * @brief A "Custom event went off" ExecutionEvent
     */
    class CustomEvent : public ExecutionEvent {

    private:
        friend class ExecutionEvent;
        /**
         * @brief Constructor
         * @param message: some custom message
         */
        explicit CustomEvent(std::shared_ptr<ExecutionControllerCustomEventMessage> message)
            : message(std::move(message)) {}

    public:
        /** @brief The message */
        std::shared_ptr<ExecutionControllerCustomEventMessage> message;

        /**
         * @brief Get a textual description of the event
         * @return a text string
         */
        std::string toString() override { return "CustomEvent"; }
    };

}// namespace wrench

/***********************/
/** \endcond           */
/***********************/


#endif//WRENCH_CUSTOM_EVENT_H

/**
 * Copyright (c) 2017-2018. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_CONSERVATIVEBACKFILLINGSCHEDULER_H
#define WRENCH_CONSERVATIVEBACKFILLINGSCHEDULER_H

#include <stdint.h>
#include "NodeAvailabilityTimeLine.h"

/***********************/
/** \cond INTERNAL     */
/***********************/


class ConservativeBackfillingScheduler {

public:

    explicit ConservativeBackfillingScheduler(u_int16_t max_num_nodes);

private:
    std::unique_ptr<NodeAvailabilityTimeLine> schedule;

};

/***********************/
/** \endcond           */
/***********************/



#endif //WRENCH_CONSERVATIVEBACKFILLINGSCHEDULER_H

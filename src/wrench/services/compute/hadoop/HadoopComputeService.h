/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_HADOOPCOMPUTESERVICE_H
#define WRENCH_HADOOPCOMPUTESERVICE_H


class HadoopComputeService {

public:

    HadoopComputeService(
            const std::string &hostname,
            const std::set<std::string> compute_resources,
            std::map<std::string, std::string> property_list,
            std::map<std::string, double> messagepayload_list
    );

    void stop();

    void runMRJob();

private:

    int main();
    bool processNextMessage();


};


#endif //WRENCH_HADOOPCOMPUTESERVICE_H

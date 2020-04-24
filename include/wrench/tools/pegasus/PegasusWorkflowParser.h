/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_PEGASUSWORKFLOWPARSER_H
#define WRENCH_PEGASUSWORKFLOWPARSER_H

#include <string>

namespace wrench {

    class Workflow;

    /**
     * @brief A class that implement methods to read workflow files 
     *        provided by the Pegasus project
     */
    class PegasusWorkflowParser {

    public:

        static Workflow *createWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

        static Workflow *createWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

        static Workflow *createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

    };

};


#endif //WRENCH_PEGASUSWORKFLOWPARSER_H

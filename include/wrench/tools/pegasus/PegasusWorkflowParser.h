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

        /**
         * @brief Method to import a Pegasus workflow in DAX format
         */
        static Workflow *createWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

        /**
         * @brief Method to import a Pegasus workflow in JSON format
         */
        static Workflow *createWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

        /**
         * @brief Method to import an executable Pegasus workflow in JSON format
         */
        static Workflow *createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies = false);

    };

};


#endif //WRENCH_PEGASUSWORKFLOWPARSER_H

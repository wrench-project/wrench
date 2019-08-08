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

    class PegasusWorkflowParser {

    public:

        static Workflow *createWorkflowFromDAXorJSON(const std::string &filename, const std::string &reference_flop_rate,
                bool redundant_dependencies, bool abstract_workflow = true);


    private:

        static Workflow *createAbstractWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies);
        static Workflow *createNonAbstractWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies);

        static Workflow *createAbstractWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies);
        static Workflow *createNonAbstractWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate, bool redundant_dependencies);

    };

};


#endif //WRENCH_PEGASUSWORKFLOWPARSER_H

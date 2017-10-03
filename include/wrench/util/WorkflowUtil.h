//
// Created by james oeth on 9/22/17.
//

#ifndef WRENCH_WORKFLOWUTIL_H
#define WRENCH_WORKFLOWUTIL_H


#include <string>
#include <vector>
#include <lemon/list_graph.h>
#include <lemon/graph_to_eps.h>
#include <pugixml.hpp>
#include "wrench/workflow/Workflow.h"


namespace wrench {
    namespace WorkflowUtil {
        void loadFromDAX(const std::string &filename, Workflow *workflow);
        //void loadFromSchema(const std::string &filename, wrench::Workflow *workflow);
        void loadFromJson(const std::string &filename, Workflow *workflow);

    }

}

#endif //WRENCH_WORKFLOWUTIL_H

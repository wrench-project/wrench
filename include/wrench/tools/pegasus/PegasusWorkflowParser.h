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
     * @brief Create an abstract workflow based on a DAX file
     *
     * @param filename: the path to the DAX file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because DAX files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the DAX file are thus assumed to be
     *                             obtained on an machine with flop rate reference_flop_rate.
     * @param redundant_dependencies: Workflows provided by Pegasus
     *                             sometimes include control/data dependencies between tasks that are already induced by
     *                             other control/data dependencies (i.e., they correspond to transitive
     *                             closures or existing edges in the workflow graphs). Passing redundant_dependencies=true
     *                             force these "redundant" dependencies to be added as edges in the workflow. Passing
     *                             redundant_dependencies=false will ignore these "redundant" dependencies. Most users
     *                             would likely pass "false". (default is false)
     * @param min_cores_per_task: If the DAX file does not specify a number of cores for a task, the minimum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     * @param max_cores_per_task: If the DAX file does not specify a number of cores for a task, the maximum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     *
     * @return a workflow
     *
     * @throw std::invalid_argument
     */
        static Workflow *createWorkflowFromDAX(const std::string &filename, const std::string &reference_flop_rate,
                                               bool redundant_dependencies = false,
                                               unsigned long min_cores_per_task = 1,
                                               unsigned long max_cores_per_task = 1);

    /**
     * @brief Create an abstract workflow based on a JSON file
     *
     * @param filename: the path to the JSON file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because JSON files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the JSON file are thus assumed to be
     *                             obtained on an machine with flop rate reference_flop_rate.
     * @param redundant_dependencies: Workflows provided by Pegasus
     *                             sometimes include control/data dependencies between tasks that are already induced by
     *                             other control/data dependencies (i.e., they correspond to transitive
     *                             closures or existing edges in the workflow graphs). Passing redundant_dependencies=true
     *                             force these "redundant" dependencies to be added as edges in the workflow. Passing
     *                             redundant_dependencies=false will ignore these "redundant" dependencies. Most users
     *                             would likely pass "false".
     * @param min_cores_per_task: If the JSON file does not specify a number of cores for a task, the minimum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     * @param max_cores_per_task: If the JSON file does not specify a number of cores for a task, the maximum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     * @return a workflow
     * @throw std::invalid_argument
     *
     */
        static Workflow *createWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                                bool redundant_dependencies = false,
                                                unsigned long min_cores_per_task = 1,
                                                unsigned long max_cores_per_task = 1);

   /**
     * @brief Create an NON-abstract workflow based on a JSON file
     *
     * @param filename: the path to the JSON file
     * @param reference_flop_rate: a reference compute speed (in flops/sec), assuming a task's computation is purely flops.
     *                             This is needed because JSON files specify task execution times in seconds,
     *                             but the WRENCH simulation needs some notion of "amount of computation" to
     *                             apply reasonable scaling. (Because the XML platform description specifies host
     *                             compute speeds in flops/sec). The times in the JSON file are thus assumed to be
     *                             obtained on an machine with flop rate reference_flop_rate.
     * @param redundant_dependencies: Workflows provided by Pegasus
     *                             sometimes include control/data dependencies between tasks that are already induced by
     *                             other control/data dependencies (i.e., they correspond to transitive
     *                             closures or existing edges in the workflow graphs). Passing redundant_dependencies=true
     *                             force these "redundant" dependencies to be added as edges in the workflow. Passing
     *                             redundant_dependencies=false will ignore these "redundant" dependencies. Most users
     *                             woudl likely pass "false".
     * @param min_cores_per_task: If the JSON file does not specify a number of cores for a task, the minimum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     * @param max_cores_per_task: If the JSON file does not specify a number of cores for a task, the maximum number of
     *                            cores on which the task can run is set to this value. (default is 1)
     *
     * @return a workflow
     * @throw std::invalid_argument
     */
        static Workflow *createExecutableWorkflowFromJSON(const std::string &filename, const std::string &reference_flop_rate,
                                                          bool redundant_dependencies = false,
                                                          unsigned long min_cores_per_task = 1,
                                                          unsigned long max_cores_per_task = 1);

    };

};


#endif //WRENCH_PEGASUSWORKFLOWPARSER_H

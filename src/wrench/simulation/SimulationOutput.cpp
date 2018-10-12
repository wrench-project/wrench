/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include "wrench/simulation/Simulation.h"
#include "wrench/simulation/SimulationOutput.h"
#include "wrench/workflow/Workflow.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <algorithm>
#include <vector>

namespace wrench {

    /**
     * \cond
     */

    /**
     * @brief Object representing an instance when a WorkflowTask was run.
     */
    typedef struct WorkflowTaskExecutionInstance {
        std::string task_id;
        unsigned long long num_cores_allocated;
        unsigned long long vertical_position;

        std::pair<double, double> whole_task;
        std::pair<double, double> read;
        std::pair<double, double> compute;
        std::pair<double, double> write;

        double failed;
        double terminated;

        std::string hostname;
        double host_flop_rate;
        double host_memory;
        unsigned long long host_num_cores;

        double getTaskEndTime() {
            return std::max({
                this->whole_task.second,
                this->failed,
                this->terminated
            });
        }
    } WorkflowTaskExecutionInstance;

    /**
     * @brief Function called by the nlohmann::json constructor when a WorkflowTaskExecutionInstance is passed in as
     *      a parameter. This returns the JSON representation of a WorkflowTaskExecutionInstance. The name of this function
     *      is important and should not be changed as it is what nlohmann expects (hardcoded in there).
     * @param j: reference to a JSON object
     * @param w: reference to a WorkflowTaskExecutionInstance
     */
    void to_json(nlohmann::json &j, const WorkflowTaskExecutionInstance &w) {
        j = nlohmann::json{
                {"task_id", w.task_id},
                {"execution_host", {
                                    {"hostname", w.hostname},
                                    {"flop_rate", w.host_flop_rate},
                                    {"memory", w.host_memory},
                                    {"cores", w.host_num_cores}

                            }},
                {"num_cores_allocated", w.num_cores_allocated},
                {"vertical_position", w.vertical_position},
                {"whole_task", {
                                    {"start", w.whole_task.first},
                                    {"end", w.whole_task.second}
                            }},
                {"read", {
                                    {"start", w.read.first},
                                    {"end", w.read.second}
                            }},
                {"compute", {
                                    {"start", w.compute.first},
                                    {"end", w.compute.second}
                            }},
                {"write", {
                                    {"start", w.write.first},
                                    {"end", w.write.second}
                            }},
                {"failed", w.failed},
                {"terminated", w.terminated}
        };
    }

    /**
     * @brief Determines if a point lies on a line segment.
     * @param segment: pair of start and end points that make up the line segment
     * @param point: point on a 1D plane
     * @return bool
     */
    bool isPointOnSegment(std::pair<unsigned long long, unsigned long long> segment, unsigned long long point) {
        return (point <= std::max(segment.first, segment.second) and point >= std::min(segment.first, segment.second));
    }

    /**
     * @brief Determines if two line segments overlap
     * @param segment1: first segment
     * @param segment2: second segment
     * @return bool
     */
    bool isSegmentOverlapping(std::pair<unsigned long long, unsigned long long> segment1, std::pair<unsigned long long, unsigned long long> segment2) {
        // segments can be touching at their endpoints
        if (segment1.second == segment2.first or segment2.second == segment1.first) {
            return false;

        // if any point of either segment lies within the other, we have overlap
        } else if (isPointOnSegment(segment1, segment2.first) or isPointOnSegment(segment1, segment2.second) or
                   isPointOnSegment(segment2, segment1.first) or isPointOnSegment(segment2, segment1.second)) {
            return true;

        // the two segments do not overlap
        } else {
            return false;
        }
    }

    /**
     * @brief Searches for a possible host utilization gantt chart layout and updates the data to include what vertical
     *        position to plot each rectangle.
     * @description Recursive backtracking search for a valid gantt chart layout. This algorithm looks for a
     *              vertical position to place each task execution event such that it doesn't overlap with
     *              any other task.
     *
     * @param data: JSON workflow execution data
     * @param index: the index of the workflow execution data up to where we would like to check for a valid layout
     * @return bool
     */
    bool searchForLayout(std::vector<WorkflowTaskExecutionInstance> &data, std::size_t index) {
        const unsigned long long PRECISION = 1000;

        WorkflowTaskExecutionInstance &current_execution_instance = data.at(index);

        auto current_rect_x_range = std::pair<unsigned long long, unsigned long long>(
                    current_execution_instance.whole_task.first * PRECISION,
                    current_execution_instance.getTaskEndTime() * PRECISION
                );

        unsigned long long num_cores_allocated = current_execution_instance.num_cores_allocated;
        unsigned long long execution_host_num_cores = current_execution_instance.host_num_cores;
        auto num_vertical_positions = execution_host_num_cores - num_cores_allocated + 1;

        /*
         * For each possible vertical position that an event can be in, we perform a check to see that its vertical
         * position doesn't make the event (a rectangle on the graph) overlap with any of the other events. If it does not
         * overlap, then we can evaluate all events up to this one with the next event in the list (recursive call). If
         * it does overlap, then we try another vertical position. If it doesn't overlap and this is the last item in the list,
         * we have found a valid layout.
         */
        for (std::size_t vertical_position = 0; vertical_position < num_vertical_positions; ++vertical_position) {

            // Set the vertical positions as we go so the entire graph layout is set when the function returns
            current_execution_instance.vertical_position = vertical_position;

            auto current_rect_y_range = std::pair<unsigned long long, unsigned long long>(
                    vertical_position,
                    vertical_position + num_cores_allocated
            );

            /*
             * We check this current event's position against all other events that were added
             * before it to make sure it doesn't overlap with any of those events.
             */
            bool has_overlap = false;
            for (std::size_t i = 0; i < index; ++i) {
                WorkflowTaskExecutionInstance other_execution_instance = data.at(i);

                // Evaluate the current event's position only against others that occurred on the same host.
                if (current_execution_instance.hostname == other_execution_instance.hostname) {
                    auto other_rect_x_range = std::pair<unsigned long long, unsigned long long>(
                            other_execution_instance.whole_task.first * PRECISION,
                            other_execution_instance.getTaskEndTime() * PRECISION
                    );

                    auto other_rect_y_range = std::pair<unsigned long long, unsigned long long>(
                            other_execution_instance.vertical_position,
                            other_execution_instance.vertical_position + other_execution_instance.num_cores_allocated
                    );

                    /*
                     * Check overlap for the x_ranges first. If there is no overlap, we can guarantee that the rectangles
                     * will not overlap. If the x_ranges do overlap, then we need to evaluate the y_ranges for overlap.
                     */
                    if (isSegmentOverlapping(current_rect_x_range, other_rect_x_range)) {
                        if (isSegmentOverlapping(current_rect_y_range, other_rect_y_range)) {
                            has_overlap = true;
                            break;
                        }
                    }
                }
            }

            if (not has_overlap and index >= data.size() - 1) {
                return true;
            } else if (not has_overlap) {
                bool found_layout = searchForLayout(data, index + 1);

                if (found_layout) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Generates graph layout for host utilization and adds that information to the JSON object.
     * @description Searches for a possible gantt chart layout to represent host utilization. If a layout is found
     *              (no tasks overlap), then information about where to plot what is added to the JSON object. Note
     *              that this is a possible layout and does not reflect what task ran on what core specifically. For
     *              example, we may hav a task that was allocated 2-cores on a idle 4-core host. The task, when plotted
     *              on the gantt chart may end up in 1 of 3 positions (using cores 0 and 1, 1 and 2, or 2 and 3).
     * @param data: JSON workflow execution data
     *
     * @throws std::runtime_error
     */
    void generateHostUtilizationGraphLayout(std::vector<WorkflowTaskExecutionInstance> &data) {
        if (searchForLayout(data, 0) == false) {
            throw std::runtime_error("SimulationOutput::generateHostUtilizationGraphLayout() could not find a valid layout.");
        }
    }

    /**
     * \endcond
     */

    /**
      * @brief Writes WorkflowTask execution history for each task to a file, formatted as a JSON array
      * @param workflow: a pointer to the Workflow
      * @param file_path: the path to write the file
      *
      * @throws std::invalid_argument
      */
    void SimulationOutput::dumpWorkflowExecutionJSON(Workflow *workflow, std::string file_path) {
        /* schema
         *  [
         *      {
         *          task_id: <id>,
         *          execution_host: {
         *              hostname: <hostname>,
         *              flop_rate: <number>,
         *              memory: <number>,
         *              cores: <number>
         *          },
         *          num_cores_allocated: <number>,
         *          vertical_position: <number>,
         *          whole_task: { start: <number>, end: <number> },
         *          read:       { start: <number>, end: <number> },
         *          compute:    { start: <number>, end: <number> },
         *          write:      { start: <number>, end: <number> },
         *          failed: <number>,
         *          terminated: <number>
         *      },
         *      .
         *      .
         *      .
         * ]
         */

        if (workflow == nullptr || file_path.empty()) {
            throw std::invalid_argument("Simulation::dumpTaskDataJSON() requires a valid workflow and file_path");
        }

        auto tasks = workflow->getTasks();

        std::vector<WorkflowTaskExecutionInstance> data;

        // For each attempted execution of a task, add a WorkflowTaskExecutionInstance to the list.
        for (auto const &task : tasks) {
            auto execution_history = task->getExecutionHistory();

            while (not execution_history.empty()) {
                auto current_task_execution = execution_history.top();

                WorkflowTaskExecutionInstance current_execution_instance;

                current_execution_instance.task_id = task->getID();

                current_execution_instance.hostname       = current_task_execution.execution_host;
                current_execution_instance.host_flop_rate =  Simulation::getHostFlopRate(current_task_execution.execution_host);
                current_execution_instance.host_memory    =  Simulation::getHostMemoryCapacity(current_task_execution.execution_host);
                current_execution_instance.host_num_cores = Simulation::getHostNumCores(current_task_execution.execution_host);

                current_execution_instance.num_cores_allocated = current_task_execution.num_cores_allocated;
                current_execution_instance.vertical_position   = 0;

                current_execution_instance.whole_task = std::make_pair(current_task_execution.task_start,         current_task_execution.task_end);
                current_execution_instance.read       = std::make_pair(current_task_execution.read_input_start,   current_task_execution.read_input_end);
                current_execution_instance.compute    = std::make_pair(current_task_execution.computation_start,  current_task_execution.computation_end);
                current_execution_instance.write      = std::make_pair(current_task_execution.write_output_start, current_task_execution.write_output_end);

                current_execution_instance.failed     = current_task_execution.task_failed;
                current_execution_instance.terminated = current_task_execution.task_terminated;

                data.push_back(current_execution_instance);
                execution_history.pop();
            }
        }

        // Set the "vertical position" of each WorkflowExecutionInstance so we know where to plot each rectangle.
        generateHostUtilizationGraphLayout(data);

        std::ofstream output(file_path);
        output << std::setw(4) << nlohmann::json(data) << std::endl;
        output.close();
    }

    /**
     * @brief Writes a JSON graph representation of the Workflow to a file
     * @description A node is added for each WorkflowFile and WorkflowTask. A directed link is added for each dependency in the Workflow.
     * @param workflow: a pointer to the workflow
     * @param file_path: the path to write the file
     *
     * @throws std::invalid_argument
     */
    void SimulationOutput::dumpWorkflowGraphJSON(wrench::Workflow *workflow, std::string file_path) {
        if (workflow == nullptr || file_path.empty()) {
            throw std::invalid_argument("Simulation::dumpTaskDataJSON() requires a valid workflow and file_path");
        }

        /* schema
         *  {
         *      "nodes": [
         *          {
         *              "type": <task>,
         *              "id": <task id>,
         *              "flops": <number>,
         *              "min_cores": <number>,
         *              "max_cores": <number>,
         *              "parallel_efficiency": <number>,
         *              "memory": <number>,
         *          },
         *          {
         *              "type": <file>,
         *              "id": <filename>,
         *              "size": <number>
         *          }
         *          .
         *          .
         *          .
         *      ],
         *
         *      "links": [
         *          {
         *              "source": <id>,
         *              "target": <id>
         *          },
         *          {
         *              "source": <id>,
         *              "target": <id>
         *          },
         *          .
         *          .
         *          .
         *      ]
         *  }
         */
        nlohmann::json nodes;
        nlohmann::json links;

        // add the task nodes
        for (const auto &task : workflow->getTasks()) {
            nodes.push_back({
                                    {"type", "task"},
                                    {"id", task->getID()},
                                    {"flops", task->getFlops()},
                                    {"min_cores", task->getMinNumCores()},
                                    {"max_cores", task->getMaxNumCores()},
                                    {"parallel_efficiency", task->getParallelEfficiency()},
                                    {"memory", task->getMemoryRequirement()}
                            });
        }

        // add the file nodes
        for (const auto &file : workflow->getFiles()) {
            nodes.push_back({
                                    {"type", "file"},
                                    {"id", file->getID()},
                                    {"size", file->getSize()}
                            });
        }

        // add the links
        for (const auto &task : workflow->getTasks()) {
            // create links between input files (if any) and the current task
            for (const auto &input_file : task->getInputFiles()) {
                links.push_back({
                                        {"source", input_file->getID()},
                                        {"target", task->getID()}
                                });
            }


            bool has_output_files = (task->getOutputFiles().size() > 0) ? true : false;
            bool has_children = (task->getNumberOfChildren() > 0) ? true : false;

            if (has_output_files) {
                // create the links between current task and its output files (if any)
                for (const auto &output_file : task->getOutputFiles()) {
                    links.push_back({{"source", task->getID()},
                                     {"target", output_file->getID()}});
                }
            } else if (has_children) {
                // then create the links from the current task to its children tasks (if it has not output files)
                for (const auto & child : workflow->getTaskChildren(task)) {
                    links.push_back({
                                            {"source", task->getID()},
                                            {"target", child->getID()}});
                }
            }
        }

        nlohmann::json workflow_task_graph;
        workflow_task_graph["nodes"] = nodes;
        workflow_task_graph["links"] = links;


        std::ofstream output(file_path);
        output << std::setw(4) << workflow_task_graph << std::endl;
        output.close();
    }
};

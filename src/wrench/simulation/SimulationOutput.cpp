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
#include "simgrid/s4u.hpp"
#include "simgrid/plugins/energy.h"

#include <wrench-dev.h>

#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include <iomanip>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <unordered_set>

#define DBL_EQUAL(x,y) (std::abs<double>((x) - (y)) < 0.1)

WRENCH_LOG_CATEGORY(wrench_core_simulation_output, "Log category for Simulation Output");

namespace wrench {


    /******************/
    /** \cond         */
    /******************/

    nlohmann::json host_utilization_layout;

    /*******************/
    /** \cond INTERNAL */
    /*******************/

    /**
     * @brief Object representing an instance when a WorkflowTask was run.
     */
    typedef struct WorkflowTaskExecutionInstance {
        /* @brief  a task ID */
        std::string task_id;
        /* @brief number of allocated cores */
        unsigned long long num_cores_allocated;
        /* @brief vertical position in gantt chart display */
        unsigned long long vertical_position;

        /* @brief task geometry in gantt chart display */
        std::pair<double, double> whole_task;
        /** @brief file read geometry in gantt chart display */
        std::pair<double, double> read;
        /* @brief computation geometry in gantt chart display */
        std::pair<double, double> compute;
        /* @brief file write geometry in gantt chart display */
        std::pair<double, double> write;

        /* @brief file read operations */
        std::vector<std::tuple<double, double, string>> reads;
        /* @brief file write operations */
        std::vector<std::tuple<double, double, string>> writes;

        /* @brief whether the task has failed */
        double failed;
        /* @brief whether the task was terminated */
        double terminated;

        /* @brief name of the host that ran the task */
        std::string hostname;
        /* @brief flop rate of the host that ran the task */
        double host_flop_rate;
        /* @brief RAM capacity of the host that ran the task */
        double host_memory;
        /* @brief number of cores of the host that ran the task */
        unsigned long long host_num_cores;

        /* 
         * @brief get the task end time
         *
         * @return a date
         */
        double getTaskEndTime() {
            return std::max({
                                    this->whole_task.second,
                                    this->failed,
                                    this->terminated
                            });
        }
    } WorkflowTaskExecutionInstance;


    /*
     * Two helper functions
     */
    static std::vector<simgrid::s4u::Host *> get_all_physical_hosts() {
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        std::vector<simgrid::s4u::Host *> hosts = simgrid_engine->get_all_hosts();
        std::vector<simgrid::s4u::Host *> to_return;

        for (auto const &h : hosts) {
            // Ignore VMs
            if (S4U_VirtualMachine::vm_to_pm_map.find(h->get_name()) != S4U_VirtualMachine::vm_to_pm_map.end()) {
                continue;
            }
            to_return.push_back(h);
        }
        return to_return;
    }

    static std::vector<simgrid::s4u::Link *> get_all_links() {
        auto simgrid_engine = simgrid::s4u::Engine::get_instance();
        return simgrid_engine->get_all_links();
    }

    /******************/
    /** \endcond      */
    /******************/


    /**
     * @brief Function that generates a unified JSON file containing the information specified by boolean arguments.
     *
     *
     *<pre>
     * JSON Structure:
     * {
     *     "disk_operations": {
     *          ...
     *      },
     *      "energy_consumption": {
     *          ...
     *      },
     *      "link_usage": {
     *          ...
     *      },
     *      "platform": {
     *          ...
     *      },
     *      "workflow_execution": {
     *          ...
     *      },
     *      "workflow_graph": {
     *          ...
     *      }
     * }
     *</pre>
     *
     * Any pieces not specified in the arguments are left out. For full structure see documentation of specific sections.
     *
     *
     *
     *
     *
     * @param workflow: a pointer to the Workflow
     * @param file_path: path for generated JSON
     * @param include_platform: boolean whether to include platform in JSON
     * @param include_workflow_exec: boolean whether to include workflow execution in JSON
     * @param include_workflow_graph: boolean whether to include workflow graph in JSON
     * @param include_energy: boolean whether to include energy consumption in JSON
     * @param generate_host_utilization_layout: boolean specifying whether or not you would like a possible host utilization
     *         layout to be generated
     * @param include_disk: boolean specifying whether to include disk operation in JSON (disk timestamps must be enabled)
     * @param include_bandwidth: boolean specifying whether to include link bandwidth measurements in JSON
     */
    void SimulationOutput::dumpUnifiedJSON(Workflow *workflow, std::string file_path,
                                           bool include_platform,
                                           bool include_workflow_exec,
                                           bool include_workflow_graph,
                                           bool include_energy,
                                           bool generate_host_utilization_layout,
                                           bool include_disk,
                                           bool include_bandwidth) {

        nlohmann::json unified_json;

        if(include_platform) {
            dumpPlatformGraphJSON(file_path, false);
            unified_json["platform"] = platform_json_part;
        }

        if(include_workflow_exec){
            dumpWorkflowExecutionJSON(workflow, file_path, generate_host_utilization_layout, false);
            unified_json["workflow_execution"] = workflow_exec_json_part;
        }

        if(include_workflow_graph){
            dumpWorkflowGraphJSON(workflow, file_path, false);
            unified_json["workflow_graph"] = workflow_graph_json_part;
        }

        if(include_energy){
            dumpHostEnergyConsumptionJSON(file_path, false);
            unified_json["energy_consumption"] = energy_json_part;
        }

        if(include_disk){
            dumpDiskOperationsJSON(file_path, false);
            unified_json["disk_operations"] = disk_json_part;
        }

        if(include_bandwidth){
            dumpLinkUsageJSON(file_path, false);
            unified_json["link_usage"] = bandwidth_json_part;
        }



        std::ofstream output(file_path);
        output << std::setw(4) << unified_json << std::endl;
        output.close();
    }

    /**
     * @brief Function called by the nlohmann::json constructor when a WorkflowTaskExecutionInstance is passed in as
     *      a parameter. This returns the JSON representation of a WorkflowTaskExecutionInstance. The name of this function
     *      is important and should not be changed as it is what nlohmann expects (hardcoded in there).
     * @param j: reference to a JSON object
     * @param w: reference to a WorkflowTaskExecutionInstance
     */
    void to_json(nlohmann::json &j, const WorkflowTaskExecutionInstance &w) {

        j["task_id"] = w.task_id;

        j["execution_host"] = {
                {"hostname", w.hostname},
                {"flop_rate", w.host_flop_rate},
                {"memory_manager_service", w.host_memory},
                {"cores", w.host_num_cores}
        };

        j["num_cores_allocated"] = w.num_cores_allocated;

        j["vertical_position"] = w.vertical_position;

        j["whole_task"] = {
                {"start",    w.whole_task.first},
                {"end",       w.whole_task.second}
        };

        nlohmann::json file_reads;
        for (auto const &r : w.reads) {
            nlohmann::json file_read = nlohmann::json::object({{"end", std::get<1>(r)},
                                                               {"start", std::get<0>(r)}, {"id", std::get<2>(r)}});
            file_reads.push_back(file_read);
        }

        j["read"] = file_reads;

        j["compute"] = {
                {"start",    w.compute.first},
                {"end",       w.compute.second}
        };


        nlohmann::json file_writes;
        for (auto const &r : w.writes) {
            nlohmann::json file_write = nlohmann::json::object({{"end", std::get<1>(r)},
                                                                {"start", std::get<0>(r)}, {"id", std::get<2>(r)}});
            file_writes.push_back(file_write);
        }

        j["write"] = file_writes;

        j["failed"] = w.failed;

        j["terminated"] = w.terminated;


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
     * @brief Determines if two line segments overlap along the x-axis and allows for a slight overlap.
     * @param segment1: first segment
     * @param segment2: second segment
     * @return bool
     */
    bool isSegmentOverlappingXAxis(std::pair<unsigned long long, unsigned long long> segment1,
                                   std::pair<unsigned long long, unsigned long long> segment2) {
        // Note: EPSILON looks big because we "blow up" the floats by 10^9 so as to do all computations
        //       with unsigned long longs instead of doubles (thus avoiding float comparison weirdnesses)
        //       And the goal of this EPSILON is to capture the fact that this is for a visual display
        //       made up of pixels, thus we don't want to be too stringent in terms of overlaps
        const unsigned long long EPSILON = 1000 * 1000 * 10;
        if (std::fabs(segment1.second - segment2.first) <= EPSILON or
            std::fabs(segment2.second - segment1.first) <= EPSILON) {
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
     * @brief Determines if two line segments overlap along the y-axis using exact values.
     * @param segment1: first segment
     * @param segment2: second segment
     * @return bool
     */
    bool isSegmentOverlappingYAxis(std::pair<unsigned long long, unsigned long long> segment1,
                                   std::pair<unsigned long long, unsigned long long> segment2) {
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
     * 
     * Recursive backtracking search for a valid gantt chart layout. This algorithm looks for a
     *              vertical position to place each task execution event such that it doesn't overlap with
     *              any other task.
     *
     * @param data: JSON workflow execution data
     * @param index: the index of the workflow execution data up to where we would like to check for a valid layout
     * @return bool
     */
    bool searchForLayout(std::vector<WorkflowTaskExecutionInstance> &data, std::size_t index) {
        const unsigned long long PRECISION = 1000 * 1000 * 1000;

        WorkflowTaskExecutionInstance &current_execution_instance = data.at(index);

        auto current_rect_x_range = std::pair<unsigned long long, unsigned long long>(
                current_execution_instance.whole_task.first * PRECISION,
                current_execution_instance.getTaskEndTime() * PRECISION
        );

        unsigned long long num_cores_allocated = current_execution_instance.num_cores_allocated;
        unsigned long long execution_host_num_cores = current_execution_instance.host_num_cores;
        auto num_vertical_positions = execution_host_num_cores - num_cores_allocated + 1;

//      std::string spaces = "";
//      for (int i=0; i < index; i++) {
//        spaces += "  ";
//      }
//      std::cout << spaces + "task = " << index <<"\n";


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

//              std::cout << spaces + "  pos = " <<  vertical_position << "\n";
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
                    if (isSegmentOverlappingXAxis(current_rect_x_range, other_rect_x_range)) {
                        if (isSegmentOverlappingYAxis(current_rect_y_range, other_rect_y_range)) {
                            has_overlap = true;
                            break;
                        }
                    }
                }
            }


            if (not has_overlap and index >= data.size() - 1) {
                host_utilization_layout[current_execution_instance.task_id] = vertical_position;
                return true;
            } else if (not has_overlap) {
                bool found_layout = searchForLayout(data, index + 1);

                if (found_layout) {
                    host_utilization_layout[current_execution_instance.task_id] = vertical_position;
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Generates graph layout for host utilization and adds that information to the JSON object.
     * 
     * Searches for a possible gantt chart layout to represent host utilization. If a layout is found
     *              (no tasks overlap), then information about where to plot what is added to the JSON object. Note
     *              that this is a possible layout and does not reflect what task ran on what core specifically. For
     *              example, we may hav a task that was allocated 2-cores on a idle 4-core host. The task, when plotted
     *              on the gantt chart may end up in 1 of 3 positions (using cores 0 and 1, 1 and 2, or 2 and 3).
     * @param data: JSON workflow execution data
     *
     * @throws std::runtime_error
     */
    void generateHostUtilizationGraphLayout(std::vector<WorkflowTaskExecutionInstance> &data) {
        if (not searchForLayout(data, 0)) {
            throw std::runtime_error(
                    "SimulationOutput::generateHostUtilizationGraphLayout() could not find a valid layout.");
        }
    }

    /**
     * \endcond
     */

    /**
      * @brief Writes WorkflowTask execution history for each task to a file, formatted as a JSON array.
      * 
      * The JSON array has the following format:
      *
      * <pre>
      *     {
      *      "workflow_execution": {
      *         "tasks": [
      *             {
      *                "compute": {
      *                     "end": <double>,
      *                     "start": <double>
      *                },
      *                "execution_host": {
      *                     "cores": <unsigned_long>,
      *                     "flop_rate": <double>,
      *                     "hostname": <string>,
      *                     "memory_manager_service": <double>
      *                },
      *                "failed": <double>,
      *                "num_cores_allocated": <unsigned_long>,
      *                "read": [
      *                     {
      *                         "end": <double>,
      *                         "start": <double>
      *                     },
      *                     {
      *                         ...
      *                     }
      *                ],
      *                "task_id": <string>,
      *                "color": <string>,
      *                "terminated": <double>,
      *                "whole_task": {
      *                     "end": <double>,
      *                     "start": <double>
      *                 },
      *                 "write": [
      *                     {
      *                         "end": <double>,
      *                         "start": <double>
      *                     },
      *                     {
      *                         ...
      *                     }
      *                 ],
      *             },
      *             {
      *                 ...
      *             }
      *             ]
      *         }
      *     }
      * </pre>
      *
      *   If generate_host_utilization_layout is set to true, a recursive function searches for a possible host
      *   utilization layout where tasks are assumed to use contiguous numbers of cores on their execution hosts.
      *   Note that each ComputeService does not enforce this, and such a layout may not exist for some workflow executions.
      *   In this situation, the function will go through the entire search space until all possible layouts are evaluated.
      *   For a large Workflow, this may take a very long time.
      *
      *   If a host utilization layout is able to be generated, the 'vertical_position' values will be set for each task run,
      *   and the task can be plotted as a rectangle on a graph where the y-axis denotes the number of cores - 1, and the x-axis denotes the
      *   workflow execution timeline. The vertical_position specifies the bottom of the rectangle. num_cores_allocated specifies the height
      *   of the rectangle.
      *
      * @param workflow: a pointer to the Workflow
      * @param file_path: the path to write the file
      * @param generate_host_utilization_layout: boolean specifying whether or not you would like a possible host utilization
      *     layout to be generated
      * @param writing_file: whether or not the file is written, true by default but will be false when utilized as part
      * of dumpUnifiedJSON
      *
      * @throws std::invalid_argument
      */
    void SimulationOutput::dumpWorkflowExecutionJSON(Workflow *workflow, std::string file_path,
                                                     bool generate_host_utilization_layout, bool writing_file) {
        if (workflow == nullptr || file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpWorkflowExecutionJSON() requires a valid workflow and file_path");
        }

        auto tasks = workflow->getTasks();
        nlohmann::json task_json;

        auto read_start_timestamps = this->getTrace<SimulationTimestampFileReadStart>();
        auto read_completion_timestamps = this->getTrace<wrench::SimulationTimestampFileReadCompletion>();
        auto read_failure_timestamps = this->getTrace<wrench::SimulationTimestampFileReadFailure>();

        auto write_start_timestamps = this->getTrace<SimulationTimestampFileWriteStart>();
        auto write_completion_timestamps = this->getTrace<wrench::SimulationTimestampFileWriteCompletion>();
        auto write_failure_timestamps = this->getTrace<wrench::SimulationTimestampFileWriteFailure>();

        std::vector<WorkflowTaskExecutionInstance> data;

        for (auto const &task : tasks) {
            auto execution_history = task->getExecutionHistory();
            while(not execution_history.empty()){
                auto current_task_execution = execution_history.top();
                WorkflowTaskExecutionInstance current_execution_instance;

                current_execution_instance.task_id = task->getID();

                if (!read_start_timestamps.empty()) {
                    for (auto & read_start_timestamp : read_start_timestamps){
                        if (read_start_timestamp->getContent()->getTask()->getID() == current_execution_instance.task_id) {
                            current_execution_instance.reads.emplace_back(read_start_timestamp->getContent()->getDate(),
                                                                          read_start_timestamp->getContent()->getEndpoint()->getDate(), read_start_timestamp->getContent()->getFile()->getID());
                        }
                    }
                }

                if (!write_start_timestamps.empty()) {
                    for (auto & write_start_timestamp : write_start_timestamps){
                        if (write_start_timestamp->getContent()->getTask()->getID() == current_execution_instance.task_id) {
                            current_execution_instance.writes.emplace_back(write_start_timestamp->getContent()->getDate(),
                                                                           write_start_timestamp->getContent()->getEndpoint()->getDate(), write_start_timestamp->getContent()->getFile()->getID());
                        }
                    }
                }

                nlohmann::json file_reads;
                for (auto const &r : current_execution_instance.reads) {
                    nlohmann::json file_read = nlohmann::json::object({{"end", std::get<1>(r)},
                                                                       {"start", std::get<0>(r)}, {"id", std::get<2>(r)}});
                    file_reads.push_back(file_read);
                }

                nlohmann::json file_writes;
                for (auto const &r : current_execution_instance.writes) {
                    nlohmann::json file_write = nlohmann::json::object({{"end", std::get<1>(r)},
                                                                        {"start", std::get<0>(r)}, {"id", std::get<2>(r)}});
                    file_writes.push_back(file_write);
                }
                task_json.push_back({
                                            {"task_id",                  task->getID()},
                                            {"color",                    task->getColor()},
                                            {"execution_host", {
                                                                                 {"hostname", current_task_execution.execution_host},
                                                                                 {"flop_rate", Simulation::getHostFlopRate(
                                                                                         current_task_execution.execution_host)},
                                                                                 {"memory_manager_service", Simulation::getHostMemoryCapacity(
                                                                                         current_task_execution.execution_host)},
                                                                                 {"cores", Simulation::getHostNumCores(
                                                                                         current_task_execution.execution_host)}
                                                                         }},
                                            {"num_cores_allocated",           current_task_execution.num_cores_allocated},
                                            {"whole_task", {
                                                                                 {"start",    current_task_execution.task_start},
                                                                                 {"end",       current_task_execution.task_end}
                                                                         }},
                                            {"read",              file_reads},
                                            {"compute",       {
                                                                                 {"start",    current_task_execution.computation_start},
                                                                                 {"end",       current_task_execution.computation_end}
                                                                         }},
                                            {"write",            file_writes},
                                            {"failed", current_task_execution.task_failed},
                                            {"terminated", current_task_execution.task_terminated}
                                    });
                execution_history.pop();
            }

        }


        // For each attempted execution of a task, add a WorkflowTaskExecutionInstance to the list.
        for (auto const &task : tasks) {
            auto execution_history = task->getExecutionHistory();


            while (not execution_history.empty()) {
                auto current_task_execution = execution_history.top();

                WorkflowTaskExecutionInstance current_execution_instance;

                current_execution_instance.task_id = task->getID();

                if (!read_start_timestamps.empty()) {
                    for (auto & read_start_timestamp : read_start_timestamps){
                        if (read_start_timestamp->getContent()->getTask()->getID() == current_execution_instance.task_id) {
                            current_execution_instance.reads.emplace_back(read_start_timestamp->getContent()->getDate(),
                                                                          read_start_timestamp->getContent()->getEndpoint()->getDate(), read_start_timestamp->getContent()->getFile()->getID());
                        }
                    }
                }

                if (!write_start_timestamps.empty()) {
                    for (auto & write_start_timestamp : write_start_timestamps){
                        if (write_start_timestamp->getContent()->getTask()->getID() == current_execution_instance.task_id) {
                            current_execution_instance.writes.emplace_back(write_start_timestamp->getContent()->getDate(),
                                                                           write_start_timestamp->getContent()->getEndpoint()->getDate(), write_start_timestamp->getContent()->getFile()->getID());
                        }
                    }
                }

                current_execution_instance.hostname = current_task_execution.execution_host;
                current_execution_instance.host_flop_rate = Simulation::getHostFlopRate(
                        current_task_execution.execution_host);
                current_execution_instance.host_memory = Simulation::getHostMemoryCapacity(
                        current_task_execution.execution_host);
                current_execution_instance.host_num_cores = Simulation::getHostNumCores(
                        current_task_execution.execution_host);

                current_execution_instance.num_cores_allocated = current_task_execution.num_cores_allocated;
                current_execution_instance.vertical_position = 0;

                current_execution_instance.whole_task = std::make_pair(current_task_execution.task_start,
                                                                       current_task_execution.task_end);
                current_execution_instance.compute = std::make_pair(current_task_execution.computation_start,
                                                                    current_task_execution.computation_end);

                current_execution_instance.failed = current_task_execution.task_failed;
                current_execution_instance.terminated = current_task_execution.task_terminated;

                data.push_back(current_execution_instance);
                execution_history.pop();
            }
        }



        // Set the "vertical position" of each WorkflowExecutionInstance so we know where to plot each rectangle
        if (generate_host_utilization_layout) {
            try {
                generateHostUtilizationGraphLayout(data);
                std::ofstream output("host_utilization_layout.json");
                output << std::setw(4) << host_utilization_layout << std::endl;
                output.close();
            } catch(std::runtime_error &e) {
                throw;
            }
        }

        nlohmann::json workflow_execution_json;
        nlohmann::json workflow_execution_json_single;
        workflow_execution_json["tasks"] = task_json;
        workflow_execution_json_single["workflow_execution"] = workflow_execution_json;
        workflow_exec_json_part = workflow_execution_json;

        if(writing_file) {
            std::ofstream output(file_path);
            output << std::setw(4) << workflow_execution_json_single << std::endl;
            output.close();
        }
    }

    /**
     * @brief Writes a JSON graph representation of the Workflow to a file.
     * 
     * A node is added for each WorkflowTask and WorkflowFile. A WorkflowTask will have the type "task" and
     *  a WorkflowFile will have the type "file". A directed link is added for each dependency in the Workflow.
     *
     * <pre>
     * {
     *      "workflow_graph": {
     *          vertices: [
     *              {
     *                  type: <"task">,
     *                  id: <string>,
     *                  flops: <double>,
     *                  min_cores: <unsigned_long>,
     *                  max_cores: <unsigned_long>,
     *                  memory_manager_service: <double>,
     *              },
     *              {
     *                  type: <"file">,
     *                  id: <string>,
     *                  size: <double>
     *              }, . . .
     *          ],
     *          edges: [
     *              {
     *                  source: <string>,
     *                  target: <string>
     *              }, . . .
     *          ]
     *      }
     *  }
     *  </pre>
     *
     * @param workflow: a pointer to the workflow
     * @param file_path: the path to write the file
     * @param writing_file: whether or not the file is written, true by default but will be false when utilized as part
     * of dumpUnifiedJSON
     *
     * @throws std::invalid_argument
     */
    void SimulationOutput::dumpWorkflowGraphJSON(wrench::Workflow *workflow, std::string file_path, bool writing_file) {
        if (workflow == nullptr || file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpWorkflowGraphJSON() requires a valid workflow and file_path");
        }

        /* schema
         *
         */
        nlohmann::json vertices;
        nlohmann::json edges;

        // add the task vertices
        for (const auto &task : workflow->getTasks()) {
            vertices.push_back({
                                       {"type",                "task"},
                                       {"id",                  task->getID()},
                                       {"flops",               task->getFlops()},
                                       {"min_cores",           task->getMinNumCores()},
                                       {"max_cores",           task->getMaxNumCores()},
                                       {"memory_manager_service",              task->getMemoryRequirement()}
                               });
        }

        // add the file vertices
        for (const auto &file : workflow->getFiles()) {
            vertices.push_back({
                                       {"type", "file"},
                                       {"id",   file->getID()},
                                       {"size", file->getSize()}
                               });
        }

        // add the edges
        for (const auto &task : workflow->getTasks()) {
            // create edges between input files (if any) and the current task
            for (const auto &input_file : task->getInputFiles()) {
                edges.push_back({
                                        {"source", input_file->getID()},
                                        {"target", task->getID()}
                                });
            }


            bool has_output_files = not task->getOutputFiles().empty();
            bool has_children = task->getNumberOfChildren() > 0;

            if (has_output_files) {
                // create the edges between current task and its output files (if any)
                for (const auto &output_file : task->getOutputFiles()) {
                    edges.push_back({{"source", task->getID()},
                                     {"target", output_file->getID()}});
                }
            } else if (has_children) {
                // then create the edges from the current task to its children tasks (if it has not output files)
                for (const auto &child : workflow->getTaskChildren(task)) {
                    edges.push_back({
                                            {"source", task->getID()},
                                            {"target", child->getID()}});
                }
            }
        }

        nlohmann::json workflow_task_graph;
        workflow_task_graph["vertices"] = vertices;
        workflow_task_graph["edges"] = edges;
        nlohmann::json workflow_graph;
        workflow_graph["workflow_graph"] = workflow_task_graph;
        workflow_graph_json_part = workflow_task_graph;

        if(writing_file) {
            std::ofstream output(file_path);
            output << std::setw(4) << nlohmann::json(workflow_graph) << std::endl;
            output.close();
        }
    }


    /**
     * @brief Writes a JSON file containing host energy consumption information as a JSON array.
     * 
     * The JSON array has the following format:
     *
     * <pre>
     * {
     *      "energy_consumption": {
     *      {
     *          hostname: <string>,
     *          pstates: [
     *              {
     *                  pstate: <int>,
     *                  idle: <double>,
     *                  epsilon: <double>,
     *                  all_cores: <double>,
     *                  speed: <double>
     *              },
     *              {
     *                  pstate: <int>,
     *                  idle: <double>,
     *                  epsilon: <double>,
     *                  all_cores: <double>,
     *                  speed: <double>
     *              } ...
     *          ],
     *          watts_off: <double>,
     *          pstate_trace: [
     *              {
     *                  time: <double>,
     *                  pstate: <int>
     *              }, ...
     *          ],
     *          consumed_energy_trace: [
     *              {
     *                  time: <double>,
     *                  joules: <double>
     *              }, ...
     *          ]
     *      }, ...
     * }
     * </pre>
     *
     * @param file_path: the path to write the file
     * @param writing_file: whether or not the file is written, true by default but will be false when utilized as part
     * of dumpUnifiedJSON
     * @throws std::invalid_argument
     * @throws std::runtime_error
     */
    void SimulationOutput::dumpHostEnergyConsumptionJSON(std::string file_path, bool writing_file) {

        if (file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpHostEnergyConsumptionJSON() requires a valid file_path");
        }

        try {

            std::vector<simgrid::s4u::Host *> hosts = get_all_physical_hosts();

            nlohmann::json hosts_energy_consumption_information;
            for (const auto &host : hosts) {
                nlohmann::json datum;

                datum["hostname"] = host->get_name();

                const char *property_string = host->get_property("wattage_per_state");
                if (property_string == nullptr) {
                    throw std::runtime_error("Host " + std::string(host->get_name()) +
                    " does not have a wattage_per_state property!");
                }
                std::string watts_per_state_property_string = std::string(property_string);
                std::vector<std::string> watts_per_state;
                boost::split(watts_per_state, watts_per_state_property_string, boost::is_any_of(","));

                for (size_t pstate = 0; pstate < watts_per_state.size(); ++pstate) {
                    std::vector<std::string> current_state_watts;
                    boost::split(current_state_watts, watts_per_state.at(pstate), boost::is_any_of(":"));

                    if (current_state_watts.size() == 2) {
                        datum["pstates"].push_back({
                                                           {"pstate",  pstate},
                                                           {"speed",   host->get_pstate_speed((int) pstate)},
                                                           {"idle",    current_state_watts.at(0)},
                                                           {"epsilon",  current_state_watts.at(0)},
                                                           {"all_cores", current_state_watts.at(1)}
                                                   });
                    } else if (current_state_watts.size() == 3) {
                        datum["pstates"].push_back({
                                                           {"pstate",    pstate},
                                                           {"speed",     host->get_pstate_speed((int) pstate)},
                                                           {"idle",      current_state_watts.at(0)},
                                                           {"epsilon",  current_state_watts.at(1)},
                                                           {"all_cores", current_state_watts.at(2)}
                                                   });
                    } else {
                        throw std::runtime_error("Host " + std::string(host->get_name()) +
                        "'s wattage_per_state property is invalid (should have 2 or 3 colon-separated numbers)");
                    }
                }

                const char *wattage_off_value = host->get_property("wattage_off");

                if (wattage_off_value != nullptr) {
                    datum["wattage_off"] = std::string(wattage_off_value);
                }

                for (const auto &pstate_timestamp : this->getTrace<SimulationTimestampPstateSet>()) {
                    if (host->get_name() == pstate_timestamp->getContent()->getHostname()) {
                        datum["pstate_trace"].push_back({
                                                                {"time",   pstate_timestamp->getDate()},
                                                                {"pstate", pstate_timestamp->getContent()->getPstate()}
                                                        });
                    }

                }

                for (const auto &energy_consumption_timestamp : this->getTrace<SimulationTimestampEnergyConsumption>()) {
                    if (host->get_name() == energy_consumption_timestamp->getContent()->getHostname()) {
                        datum["consumed_energy_trace"].push_back({
                                                                         {"time",   energy_consumption_timestamp->getDate()},
                                                                         {"joules", energy_consumption_timestamp->getContent()->getConsumption()}
                                                                 });
                    }
                }

                hosts_energy_consumption_information.push_back(datum);
            }

            // std::cerr << hosts_energy_consumption_information.dump(4);


            nlohmann::json energy_consumption;
            energy_consumption["energy_consumption"] = hosts_energy_consumption_information;
            energy_json_part = hosts_energy_consumption_information;

            if(writing_file) {
                std::ofstream output(file_path);
                output << std::setw(4) << nlohmann::json(energy_consumption) << std::endl;
                output.close();
            }

        } catch (std::runtime_error &e) {
            // Just re-throw
            throw;
        }
    }

    /**
     * @brief Writes a JSON file containing all hosts, network links, and the routes between each host.
     * 
     * The JSON array has the following format:
     *
     * <pre>
     * {
     *     "platform":{
     *       vertices: [
     *            {
     *                type: <"host">,
     *                id: <string>,
     *              flop_rate: <double (flops per second)>,
     *              memory_manager_service: <double (bytes)>,
     *              cores: <unsigned_long>
     *           },
     *           {
     *                type: <"link">,
     *                id: <string>,
     *               bandwidth: <double (bytes per second)>,
     *             latency: <double (in seconds)>
     *            }, . . .
     *       ],
     *        edges: [
     *            {
     *                source: {
     *                    type: <string>,
     *                    id: <string>
     *              }
     *                target: {
     *                    type: <string>,
     *                    id: <string>
     *            }
     *            }, . . .
     *      ],
     *      routes: [
     *           {
     *               source: <string>,
     *               target: <string>,
     *               latency: <double (in seconds)>
     *               route: [
     *                  link_id, ...
     *              ]
     *        }
     *    ],
     *   }
     * }
     * </pre>
     *
     * @param file_path: the path to write the file
     * @param writing_file: whether or not the file is written, true by default but will be false when utilized as part
     * of dumpUnifiedJSON
     *
     * @throws std::invalid_argument
     */
    void SimulationOutput::dumpPlatformGraphJSON(std::string file_path, bool writing_file) {
        if (file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpPlatformGraphJSON() requires a valid file_path");
        }

        nlohmann::json platform_graph_json;

//        simgrid::s4u::Engine *simgrid_engine = simgrid::s4u::Engine::get_instance();

        // Get all the hosts
        auto hosts = get_all_physical_hosts();

        // get the by-cluster host information
        std::map<std::string, std::vector<std::string>> cluster_to_hosts = S4U_Simulation::getAllHostnamesByCluster();

        // Build a host-to-cluster map initialized with hostnames as cluster_ids
        std::map<std::string, std::string> host_to_cluster;
        for (auto const &h : hosts) {
            host_to_cluster[h->get_name()] = h->get_name();
        }
        // Update cluster_id value for those hosts that are in an actual cluster
        for (auto const &c : cluster_to_hosts) {
            std::string cluster_id = c.first;
            for (auto const &h : c.second) {
                host_to_cluster[h] = cluster_id;
            }
        }

        // add all hosts to the list of vertices
        for (const auto &host : hosts) {
            platform_graph_json["vertices"].push_back({
                                                              {"type",       "host"},
                                                              {"id",         host->get_name()},
                                                              {"cluster_id", host_to_cluster[host->get_name()]},
                                                              {"flop_rate",  host->get_speed()},
                                                              {"memory_manager_service",     Simulation::getHostMemoryCapacity(
                                                                      host->get_name())},
                                                              {"cores",      host->get_core_count()}
                                                      });
        }

        // add all network links to the list of vertices
        std::vector<simgrid::s4u::Link *> links = get_all_links();
        for (const auto &link : links) {
            if (not(link->get_name() == "__loopback__")) { // Ignore loopback link
                platform_graph_json["vertices"].push_back({
                                                                  {"type",      "link"},
                                                                  {"id",        link->get_name()},
                                                                  {"bandwidth", link->get_bandwidth()},
                                                                  {"latency",   link->get_latency()}
                                                          });
            }
        }


        // add each route to the list of routes
        std::vector<simgrid::s4u::Link *> route_forward;
        std::vector<simgrid::s4u::Link *> route_backward;
        double route_forward_latency = 0;
        double route_backward_latency = 0;

        // for every combination of host pairs
        for (auto target = hosts.begin(); target != hosts.end(); ++target) {
            for (auto source = hosts.begin(); source != target; ++source) {
                nlohmann::json route_forward_json;


                // populate "route_forward" with an ordered list of network links along
                // the route between source and target
                (*source)->route_to(*target, route_forward, &route_forward_latency);

                // add the route from source to target to the json
                route_forward_json["source"] = (*source)->get_name();
                route_forward_json["target"] = (*target)->get_name();
                route_forward_json["latency"] = route_forward_latency;

                for (const auto &link : route_forward) {
                    route_forward_json["route"].push_back(link->get_name());
                }

                platform_graph_json["routes"].push_back(route_forward_json);

                // populate "route_backward" with an ordered list of network links along
                // the route between target and source; the "route_backward" could be different
                // so we need to add it if it is in fact different
                nlohmann::json route_backward_json;
                (*target)->route_to(*source, route_backward, &route_backward_latency);

                if (route_forward.empty() and route_backward.empty()) {
                    throw std::invalid_argument("Cannot generate platform graph because no route is found between hosts " +
                                                std::string((*source)->get_cname()) + " and " +
                                                std::string((*target)->get_cname()));
                }

                // check to see if the route from source to target is the same as from target to source
                bool is_route_equal = true;
                if (route_forward.size() == route_backward.size()) {
                    for (size_t i = 0; i < route_forward.size(); ++i) {
                        if (route_forward.at(i)->get_name() !=
                            route_backward.at(route_backward.size() - 1 - i)->get_name()) {
                            is_route_equal = false;
                            break;
                        }
                    }
                } else {
                    is_route_equal = false;
                }

                if (not is_route_equal) {
                    // add the route from target to source to the json
                    route_backward_json["source"] = (*target)->get_name();
                    route_backward_json["target"] = (*source)->get_name();
                    route_backward_json["latency"] = route_backward_latency;

                    for (const auto &link : route_backward) {
                        route_backward_json["route"].push_back(link->get_name());
                    }

                    platform_graph_json["routes"].push_back(route_backward_json);
                }

                // reset these values
                route_forward.clear();
                route_backward.clear();

                // reset these values
                route_forward_latency = 0;
                route_backward_latency = 0;
            }
        }

        // maintain a unique list of edges where edges are represented using the following string format:
        // <source_type>:<source_id>-<target_type>:<target_id> where type could be 'host' or 'link'
        std::unordered_set<std::string> edges;
        const std::string HOST("host");
        const std::string LINK("link");

        std::string source_string;
        std::string target_string;

        std::string source_id;
        std::string target_id;

        // for each route, add "host<-->link" and "link<-->link" connections
        for (nlohmann::json::iterator route_itr = platform_graph_json["routes"].begin();
             route_itr != platform_graph_json["routes"].end(); ++route_itr) {

            source_id = (*route_itr)["source"].get<std::string>();
            source_string = HOST + ":" + source_id;

            target_id = (*route_itr)["route"].at(0).get<std::string>();
            target_string = LINK + ":" + target_id;

            // check that the undirected edge doesn't already exist in set of edges
            if (edges.find(source_string + "-" + target_string) == edges.end() and
                edges.find(target_string + "-" + source_string) == edges.end()) {

                edges.insert(source_string + "-" + target_string);

                // add a graph link from the source host to the first network link
                platform_graph_json["edges"].push_back({
                                                               {"source", {
                                                                                  {"type", HOST},
                                                                                  {"id", source_id}
                                                                          }

                                                               },
                                                               {"target", {
                                                                                  {"type", LINK},
                                                                                  {"id", target_id}
                                                                          }
                                                               }
                                                       });
            }



            // add graph edges comprising only network links
            for (nlohmann::json::iterator link_itr = (*route_itr)["route"].begin();
                 link_itr != (*route_itr)["route"].end(); ++link_itr) {
                auto next_link_itr = link_itr + 1;

                if (next_link_itr != (*route_itr)["route"].end()) {

                    source_id = (*link_itr).get<std::string>();
                    source_string = LINK + ":" + source_id;

                    target_id = (*next_link_itr).get<std::string>();
                    target_string = LINK + ":" + target_id;

                    // check that the undirected edge doesn't already exist in set of edges
                    if (edges.find(source_string + "-" + target_string) == edges.end() and
                        edges.find(target_string + "-" + source_string) == edges.end()) {

                        edges.insert(source_string + "-" + target_string);

                        platform_graph_json["edges"].push_back({
                                                                       {"source", {
                                                                                          {"type", LINK},
                                                                                          {"id", source_id}
                                                                                  }
                                                                       },
                                                                       {"target", {
                                                                                          {"type", LINK},
                                                                                          {"id", target_id}
                                                                                  }
                                                                       }
                                                               });
                    }
                }
            }

            source_id = (*route_itr)["route"].at(((*route_itr)["route"].size()) - 1).get<std::string>();
            source_string = LINK + ":" + source_id;

            target_id = (*route_itr)["target"].get<std::string>();
            target_string = HOST + ":" + target_id;

            // check that the undirected edge doesn't already exist in set of edges
            if (edges.find(source_string + "-" + target_string) == edges.end() and
                edges.find(target_string + "-" + source_string) == edges.end()) {

                edges.insert(source_string + "-" + target_string);

                // add a graph link from the last link to the target host
                platform_graph_json["edges"].push_back({
                                                               {"source", {
                                                                                  {"type", "link"},
                                                                                  {"id", source_id}
                                                                          }
                                                               },
                                                               {"target", {
                                                                                  {"type", "host"},
                                                                                  {"id", target_id}
                                                                          }
                                                               }
                                                       });

            }
        }

        //std::cerr << platform_graph_json.dump(4) << std::endl;
        nlohmann::json platform;
        platform["platform"] = platform_graph_json;
        platform_json_part = platform_graph_json;

        if(writing_file){
            std::ofstream output(file_path);
            output << std::setw(4) << nlohmann::json(platform) << std::endl;
            output.close();
        }
    }

    /**
     *
     * @brief Writes a JSON file containing disk operation information as a JSON array.
     *
     * >>>>>NOTE<<<<< The timestamps the JSON is generated from are disabled by default.
     * Enable them with SimulationOutput::enableDiskTimestamps() to use.
     *
     * The JSON array has the following format:
     *
     * <pre>
     * {
     *  "disk_operations": {
     *      "io_host": {                        <--- Hostname
     *          "/": {                          <--- Mount
     *             "reads": [
     *                  {
     *                   "bytes": <double>,
     *                   "end": <double>,
     *                   "failed": <double>,
     *                   "start": <double>
     *                  },
     *                  {
     *                     ...
     *                  }
     *                  ],
     *             "writes": [
     *                 {
     *                  "bytes": <double>,
     *                  "end": <double>,
     *                  "failed": <double>,
     *                  "start": <double>
     *                  },
     *                  {
     *                   ...
     *                  }
     *                  ]
     *              }
     *          }
     *   }
     * }
     * </pre>
     *
     * @param file_path - path to save JSON at
     * @param writing_file - boolean, default true, to write the JSON to the specified file path. Used for unified output.
     *
     * @throws invalid_argument
     */
    void SimulationOutput::dumpDiskOperationsJSON(std::string file_path, bool writing_file) {
        if (file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpDiskOperationJSON() requires a valid file_path");
        }
        nlohmann::json disk_operations_json;

        auto read_start_timestamps = this->getTrace<SimulationTimestampDiskReadStart>();
        auto read_completion_timestamps = this->getTrace<wrench::SimulationTimestampDiskReadCompletion>();
        auto read_failure_timestamps = this->getTrace<wrench::SimulationTimestampDiskReadFailure>();

        auto write_start_timestamps = this->getTrace<SimulationTimestampDiskWriteStart>();
        auto write_completion_timestamps = this->getTrace<wrench::SimulationTimestampDiskWriteCompletion>();
        auto write_failure_timestamps = this->getTrace<wrench::SimulationTimestampDiskWriteFailure>();


        std::set<std::string> hostnames;

        //std::tuple<string, string, std::tuple<double, double, double>> disk_operation;
        std::tuple<double, double, double> disk_operation;

        if(!read_start_timestamps.empty()){
            for (auto & timestamp : read_start_timestamps) {
                hostnames.insert(timestamp->getContent()->getHostname());
            }
        }
        if(!write_start_timestamps.empty()){
            for (auto & timestamp : write_start_timestamps) {
                hostnames.insert(timestamp->getContent()->getHostname());
            }
        }



        for(auto & host : hostnames) {
            std::set<std::string> mounts;
            if(!read_start_timestamps.empty()){
                for (auto & timestamp : read_start_timestamps) {
                    if(timestamp->getContent()->getHostname().compare(host) == 0){
                        mounts.insert(timestamp->getContent()->getMount());
                    }
                }
            }
            if(!write_start_timestamps.empty()){
                for (auto & timestamp : write_start_timestamps) {
                    if(timestamp->getContent()->getHostname().compare(host) == 0){
                        mounts.insert(timestamp->getContent()->getMount());
                    }
                }
            }
            for (auto & mount : mounts) {
                std::vector<std::tuple<double, double, double>> reads;
                std::vector<std::tuple<double, double, double>> writes;

                if (!read_start_timestamps.empty()) {
                    for (auto & read_start_timestamp : read_start_timestamps){
                        if (read_start_timestamp->getContent()->getHostname().compare(host) == 0 && read_start_timestamp->getContent()->getMount().compare(mount) == 0){
                            disk_operation = std::make_tuple(read_start_timestamp->getContent()->getDate(),
                                                             read_start_timestamp->getContent()->getEndpoint()->getDate(),
                                                             read_start_timestamp->getContent()->getBytes());
                            reads.emplace_back(disk_operation);
                        }

                    }
                }
                if (!write_start_timestamps.empty()) {
                    for (auto & write_start_timestamp : write_start_timestamps){
                        if (write_start_timestamp->getContent()->getHostname().compare(host) == 0 && write_start_timestamp->getContent()->getMount().compare(mount) == 0){
                            disk_operation = std::make_tuple(write_start_timestamp->getContent()->getDate(),
                                                             write_start_timestamp->getContent()->getEndpoint()->getDate(),
                                                             write_start_timestamp->getContent()->getBytes());
                            writes.emplace_back(disk_operation);
                        }

                    }
                }

                nlohmann::json disk_reads;
                for (auto const &r : reads) {
                    nlohmann::json disk_read = nlohmann::json::object({{"start", std::get<0>(r)},
                                                                       {"end", std::get<1>(r)},
                                                                       {"bytes",std::get<2>(r)},
                                                                       {"failed", "-1"}});
                    disk_reads.push_back(disk_read);
                }
                if(!read_failure_timestamps.empty()){
                    for (auto & timestamp : read_failure_timestamps) {
                        for (auto & disk_read : disk_reads){
                            if(timestamp->getContent()->getDate() == disk_read["end"] && timestamp->getContent()->getEndpoint()->getDate() == disk_read["start"]){
                                disk_read["failed"] = "1";
                            }
                        }
                    }
                }

                nlohmann::json disk_writes;
                for (auto const &w : writes) {
                    nlohmann::json disk_write = nlohmann::json::object({{"start", std::get<0>(w)},
                                                                        {"end", std::get<1>(w)},
                                                                        {"bytes",std::get<2>(w)},
                                                                        {"failed", "-1"}});
                    disk_writes.push_back(disk_write);
                }
                if(!write_failure_timestamps.empty()){
                    for (auto & timestamp : write_failure_timestamps) {
                        for (auto & disk_write : disk_writes){
                            if(timestamp->getContent()->getDate() == disk_write["end"] && timestamp->getContent()->getEndpoint()->getDate() == disk_write["start"]){
                                disk_write["failed"] = "1";
                            }
                        }
                    }
                }

                disk_operations_json[host][mount]["reads"] = disk_reads;
                disk_operations_json[host][mount]["writes"] = disk_writes;
            }



        }

        disk_json_part = disk_operations_json;

        if(writing_file) {
            std::ofstream output(file_path);
            output << std::setw(4) << disk_operations_json << std::endl;
            output.close();
        }
    }

    /** Writes a JSON file containing link usage information as a JSON array.
     *
     * This information will not be generated without using the bandwidth meter service and providing it with linknames
     * to monitor.
     *
     *<pre>
     * {
     *  "link_usage": {
     *      "links": [
     *                  {
     *                   "link_usage_trace": [
     *                          {
     *                           "bytes per second": <double>,
     *                           "time": <double>
     *                          },
     *                          {
     *                              ...
     *                          },
     *                      ],
     *                      "linkname": <string>
     *                  },
     *                  {
     *                      ...
     *                  }
     *              ]
     *   }
     * }
     * </pre>
     *
     *
     * @param file_path: path where json file is written
     * @param writing_file: whether to write file to disk. Enabled by default.
     *
     * @throws std::invalid_argument
     * @throws std::runtime_error
     */
    void SimulationOutput::dumpLinkUsageJSON(std::string file_path, bool writing_file) {
        if (file_path.empty()) {
            throw std::invalid_argument("SimulationOutput::dumpLinkUsageJSON() requires a valid file_path");
        }

        nlohmann::json bandwidth_json;

        try {
            auto simgrid_engine = simgrid::s4u::Engine::get_instance();
            std::vector<simgrid::s4u::Link *> links = get_all_links();

            for( const auto &link: links) {
                nlohmann::json datum;
                datum["linkname"] = link->get_name();

                for (const auto &link_usage_timestamp : this->getTrace<SimulationTimestampLinkUsage>()) {
                    if (link->get_name() == link_usage_timestamp->getContent()->getLinkname()) {
                        datum["link_usage_trace"].push_back({
                                                                    {"time",   link_usage_timestamp->getDate()},
                                                                    {"bytes per second", link_usage_timestamp->getContent()->getUsage()}
                                                            });
                    }
                }

                bandwidth_json.push_back(datum);
            }

            nlohmann::json link_usage;
            nlohmann::json links_list;
            links_list["links"] = bandwidth_json;
            link_usage["link_usage"] = links_list;
            bandwidth_json_part = links_list;

            if(writing_file) {
                std::ofstream output(file_path);
                output << std::setw(4) << link_usage << std::endl;
                output.close();
            }
        } catch (std::runtime_error &e) {
            throw;
        }
    }


    /**
     * @brief Destructor
     */
    SimulationOutput::~SimulationOutput() {
        for (auto t : this->traces) {
            delete t.second;
        }
        this->traces.clear();
    }

    /**
     * @brief Constructor
     */
    SimulationOutput::SimulationOutput()  {
        // By default enable all task timestamps
        this->setEnabled<SimulationTimestampTaskStart>(true);
        this->setEnabled<SimulationTimestampTaskFailure>(true);
        this->setEnabled<SimulationTimestampTaskCompletion>(true);
        this->setEnabled<SimulationTimestampTaskTermination>(true);

        // By default enable all file read timestamps
        this->setEnabled<SimulationTimestampFileReadStart>(true);
        this->setEnabled<SimulationTimestampFileReadFailure>(true);
        this->setEnabled<SimulationTimestampFileReadCompletion>(true);

        // By default enable all file write timestamps
        this->setEnabled<SimulationTimestampFileWriteStart>(true);
        this->setEnabled<SimulationTimestampFileWriteFailure>(true);
        this->setEnabled<SimulationTimestampFileWriteCompletion>(true);

        //By default disable (for now) all disk read timestamps
        this->setEnabled<SimulationTimestampDiskReadStart>(false);
        this->setEnabled<SimulationTimestampDiskReadFailure>(false);
        this->setEnabled<SimulationTimestampDiskReadCompletion>(false);

        // By default disable (for now) all disk write timestamps
        this->setEnabled<SimulationTimestampDiskWriteStart>(false);
        this->setEnabled<SimulationTimestampDiskWriteFailure>(false);
        this->setEnabled<SimulationTimestampDiskWriteCompletion>(false);

        // By default enable all file copy timestamps
        this->setEnabled<SimulationTimestampFileCopyStart>(true);
        this->setEnabled<SimulationTimestampFileCopyFailure>(true);
        this->setEnabled<SimulationTimestampFileCopyCompletion>(true);

        // By default enable all power timestamps
        this->setEnabled<SimulationTimestampPstateSet>(true);
        this->setEnabled<SimulationTimestampEnergyConsumption>(true);

        // By default enable all link usage timestamps
        this->setEnabled<SimulationTimestampLinkUsage>(true);
    }

    /**
     * @brief Add a task start timestamp
     * @param task: a workflow task
     */
    void SimulationOutput::addTimestampTaskStart(WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampTaskStart>()) {
            this->addTimestamp<SimulationTimestampTaskStart>(new SimulationTimestampTaskStart(task));
        }
    }

    /**
     * @brief Add a task start failure
     * @param task: a workflow task
     */
    void SimulationOutput::addTimestampTaskFailure(WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampTaskFailure>()) {
            this->addTimestamp<SimulationTimestampTaskFailure>(new SimulationTimestampTaskFailure(task));
        }
    }

    /**
     * @brief Add a task start completion
     * @param task: a workflow task
     */
    void SimulationOutput::addTimestampTaskCompletion(WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampTaskCompletion>()) {
            this->addTimestamp<SimulationTimestampTaskCompletion>(new SimulationTimestampTaskCompletion(task));
        }
    }

    /**
    * @brief Add a task start termination
    * @param task: a workflow task
    */
    void SimulationOutput::addTimestampTaskTermination(WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampTaskTermination>()) {
            this->addTimestamp<SimulationTimestampTaskTermination>(new SimulationTimestampTaskTermination(task));
        }
    }

    /**
     * @brief Add a file read start timestamp
     * @param file: a workflow file
     * @param src: the source location
     * @param service: the source storage service
     * @param task: the workflow task for which this read is done (or nullptr);
     */
    void SimulationOutput::addTimestampFileReadStart(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileReadStart>()) {
            this->addTimestamp<SimulationTimestampFileReadStart>(new SimulationTimestampFileReadStart(file, src, service, task));
        }
    }

    /**
    * @brief Add a file read failure timestamp
    * @param file: a workflow file
    * @param src: the source location
    * @param service: the source storage service
    * @param task: the workflow task for which this read is done (or nullptr);
    */
    void SimulationOutput::addTimestampFileReadFailure(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileReadFailure>()) {
            this->addTimestamp<SimulationTimestampFileReadFailure>(new SimulationTimestampFileReadFailure(file, src, service, task));
        }
    }

    /**
    * @brief Add a file read completion timestamp
    * @param file: a workflow file
    * @param src: the source location
    * @param service: the source storage service
    * @param task: the workflow task for which this read is done (or nullptr);
    */
    void SimulationOutput::addTimestampFileReadCompletion(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileReadCompletion>()) {
            this->addTimestamp<SimulationTimestampFileReadCompletion>(new SimulationTimestampFileReadCompletion(file, src, service, task));
        }
    }

    /**
     * @brief Add a file write start timestamp
     * @param file: a workflow file
     * @param src: the target location
     * @param service: the target storage service
     * @param task: the workflow task for which this write is done (or nullptr);
     */
    void SimulationOutput::addTimestampFileWriteStart(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileWriteStart>()) {
            this->addTimestamp<SimulationTimestampFileWriteStart>(new SimulationTimestampFileWriteStart(file, src, service, task));
        }
    }

    /**
    * @brief Add a file write failure timestamp
    * @param file: a workflow file
    * @param src: the target location
    * @param service: the target storage service
    * @param task: the workflow task for which this write is done (or nullptr);
    */
    void SimulationOutput::addTimestampFileWriteFailure(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileWriteFailure>()) {
            this->addTimestamp<SimulationTimestampFileWriteFailure>(new SimulationTimestampFileWriteFailure(file, src, service, task));
        }
    }

    /**
    * @brief Add a file write completion timestamp
    * @param file: a workflow file
    * @param src: the target location
    * @param service: the target storage service
    * @param task: the workflow task for which this write is done (or nullptr);
    */
    void SimulationOutput::addTimestampFileWriteCompletion(WorkflowFile *file, FileLocation *src, StorageService *service, WorkflowTask *task) {
        if (this->isEnabled<SimulationTimestampFileWriteCompletion>()) {
            this->addTimestamp<SimulationTimestampFileWriteCompletion>(new SimulationTimestampFileWriteCompletion(file, src, service, task));
        }
    }



    /**
     * @brief Add a file copy start timestamp
     * @param file: a workflow file
     * @param src: the source location
     * @param dst: the target location
     */
    void SimulationOutput::addTimestampFileCopyStart(WorkflowFile *file, std::shared_ptr<FileLocation> src,
                                                     std::shared_ptr<FileLocation> dst) {
        if (this->isEnabled<SimulationTimestampFileCopyStart>()) {
            this->addTimestamp<SimulationTimestampFileCopyStart>(new SimulationTimestampFileCopyStart(file, src, dst));
        }
    }

    /**
     * @brief Add a file copy failure timestamp
     * @param file: a workflow file
     * @param src: the source location
     * @param dst: the target location
     */
    void SimulationOutput::addTimestampFileCopyFailure(WorkflowFile *file, std::shared_ptr<FileLocation> src,
                                                       std::shared_ptr<FileLocation> dst) {
        if (this->isEnabled<SimulationTimestampFileCopyFailure>()) {
            this->addTimestamp<SimulationTimestampFileCopyFailure>(new SimulationTimestampFileCopyFailure(file, src, dst));
        }
    }

    /**
     * @brief Add a file copy completion timestamp
     * @param file: a workflow file
     * @param src: the source location
     * @param dst: the target location
     */
    void SimulationOutput::addTimestampFileCopyCompletion(WorkflowFile *file, std::shared_ptr<FileLocation> src,
                                                          std::shared_ptr<FileLocation> dst) {
        if (this->isEnabled<SimulationTimestampFileCopyCompletion>()) {
            this->addTimestamp<SimulationTimestampFileCopyCompletion>(new SimulationTimestampFileCopyCompletion(file, src, dst));
        }
    }

    /**
 * @brief Add a file read start timestamp
 * @param hostname: hostname being read from
 * @param mount: mountpoint of disk
 * @param bytes: number of bytes read
 * @param unique_sequence_number: an integer id
 */
    void SimulationOutput::addTimestampDiskReadStart(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskReadStart>()) {
            this->addTimestamp<SimulationTimestampDiskReadStart>(new SimulationTimestampDiskReadStart(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
     * @brief Add a file read failure timestamp
     * @param hostname: hostname being read from
     * @param mount: mountpoint of disk
     * @param bytes: number of bytes read
     * @param unique_sequence_number: an integer id
     */
    void SimulationOutput::addTimestampDiskReadFailure(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskReadFailure>()) {
            this->addTimestamp<SimulationTimestampDiskReadFailure>(new SimulationTimestampDiskReadFailure(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
     * @brief Add a file read completion timestamp
     * @param hostname: hostname being read from
     * @param mount: mountpoint of disk
     * @param bytes: number of bytes read
     * @param unique_sequence_number: an integer id
     */
    void SimulationOutput::addTimestampDiskReadCompletion(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskReadCompletion>()) {
            this->addTimestamp<SimulationTimestampDiskReadCompletion>(new SimulationTimestampDiskReadCompletion(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
     * @brief Add a file write start timestamp
     * @param hostname: hostname being read from
     * @param mount: mountpoint of disk
     * @param bytes: number of bytes read
     * @param unique_sequence_number: an integer id
     */
    void SimulationOutput::addTimestampDiskWriteStart(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskWriteStart>()) {
            this->addTimestamp<SimulationTimestampDiskWriteStart>(new SimulationTimestampDiskWriteStart(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
     * @brief Add a file write failure timestamp
     * @param hostname: hostname being read from
     * @param mount: mountpoint of disk
     * @param bytes: number of bytes read
     * @param unique_sequence_number: an integer id
     */
    void SimulationOutput::addTimestampDiskWriteFailure(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskWriteFailure>()) {
            this->addTimestamp<SimulationTimestampDiskWriteFailure>(new SimulationTimestampDiskWriteFailure(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
    * @brief Add a file write completion timestamp
    * @param hostname: hostname being read from
    * @param mount: mountpoint of disk
    * @param bytes: number of bytes read
    * @param unique_sequence_number: an integer id
    */
    void SimulationOutput::addTimestampDiskWriteCompletion(std::string hostname, std::string mount, double bytes, int unique_sequence_number) {
        if (this->isEnabled<SimulationTimestampDiskWriteCompletion>()) {
            this->addTimestamp<SimulationTimestampDiskWriteCompletion>(new SimulationTimestampDiskWriteCompletion(hostname, mount, bytes, unique_sequence_number));
        }
    }

    /**
     * @brief Add a pstate change/set timestamp
     * @param hostname: a hostname
     * @param pstate: a pstate index
     */
    void SimulationOutput::addTimestampPstateSet(std::string hostname, int pstate) {
        if (this->isEnabled<SimulationTimestampPstateSet>()) {
            this->addTimestamp<SimulationTimestampPstateSet>(new SimulationTimestampPstateSet(hostname, pstate));
        }
    }

    /**
     * @brief Add an energy consumption timestamp
     * @param hostname: a hostname
     * @param joules: consumption in joules
     */
    void SimulationOutput::addTimestampEnergyConsumption(std::string hostname, double joules) {
        static std::unordered_map<std::string, std::vector<SimulationTimestampEnergyConsumption*>> last_two_timestamps;

        if (not this->isEnabled<SimulationTimestampEnergyConsumption>()) {
            return;
        }

        auto new_timestamp = new SimulationTimestampEnergyConsumption(hostname,joules);

        // If less thant 2 time-stamp for that host, just record and add
        if (last_two_timestamps[hostname].size()  < 2) {
            last_two_timestamps[hostname].push_back(new_timestamp);
            this->addTimestamp<SimulationTimestampEnergyConsumption>(new_timestamp);
            return;
        }

        // Otherwise, check whether we can merge
        bool can_merge = DBL_EQUAL(last_two_timestamps[hostname].at(0)->getConsumption(), last_two_timestamps[hostname].at(1)->getConsumption()) and
                         DBL_EQUAL(last_two_timestamps[hostname].at(1)->getConsumption(), new_timestamp->getConsumption());

        if (can_merge) {
            last_two_timestamps[hostname].at(1)->setDate(new_timestamp->getDate());
        } else {
            last_two_timestamps[hostname][0] = last_two_timestamps[hostname][1];
            last_two_timestamps[hostname][1] = new_timestamp;
            this->addTimestamp<SimulationTimestampEnergyConsumption>(new_timestamp);
        }

    }

    /**
     * @brief Add a link usage timestamp
     * @param linkname: a linkname
     * @param bytes_per_second: link usage in bytes_per_second
     */
    void SimulationOutput::addTimestampLinkUsage(std::string linkname, double bytes_per_second) {
        static std::unordered_map<std::string, std::vector<SimulationTimestampLinkUsage*>> last_two_timestamps;

        if (not this->isEnabled<SimulationTimestampLinkUsage>()) {
            return;
        }

        auto new_timestamp = new SimulationTimestampLinkUsage(linkname, bytes_per_second);

        // If less thant 2 time-stamp for that link, just record and add
        if (last_two_timestamps[linkname].size()  < 2) {
            last_two_timestamps[linkname].push_back(new_timestamp);
            this->addTimestamp<SimulationTimestampLinkUsage>(new_timestamp);
            return;
        }

        // Otherwise, check whether we can merge
        bool can_merge = DBL_EQUAL(last_two_timestamps[linkname].at(0)->getUsage(), last_two_timestamps[linkname].at(1)->getUsage()) and
                         DBL_EQUAL(last_two_timestamps[linkname].at(1)->getUsage(), new_timestamp->getUsage());

        if (can_merge) {
            last_two_timestamps[linkname].at(1)->setDate(new_timestamp->getDate());
        } else {
            last_two_timestamps[linkname][0] = last_two_timestamps[linkname][1];
            last_two_timestamps[linkname][1] = new_timestamp;
            this->addTimestamp<SimulationTimestampLinkUsage>(new_timestamp);
        }

    }

    /**
     * @brief Enable or Disable the insertion of task-related timestamps in
     *        the simulation output (enabled by default)
     * @param enabled true to enable, false to disable
     */
    void SimulationOutput::enableWorkflowTaskTimestamps(bool enabled) {
        this->setEnabled<SimulationTimestampTaskStart>(enabled);
        this->setEnabled<SimulationTimestampTaskFailure>(enabled);
        this->setEnabled<SimulationTimestampTaskCompletion>(enabled);
        this->setEnabled<SimulationTimestampTaskTermination>(enabled);
    }

    /**
     * @brief Enable or Disable the insertion of disk-related timestamps in
     *        the simulation output (enabled by default)
     * @param enabled true to enable, false to disable
     */
    void SimulationOutput::enableDiskTimestamps(bool enabled) {
        this->setEnabled<SimulationTimestampDiskReadStart>(enabled);
        this->setEnabled<SimulationTimestampDiskReadFailure>(enabled);
        this->setEnabled<SimulationTimestampDiskReadCompletion>(enabled);
        this->setEnabled<SimulationTimestampDiskWriteStart>(enabled);
        this->setEnabled<SimulationTimestampDiskWriteFailure>(enabled);
        this->setEnabled<SimulationTimestampDiskWriteCompletion>(enabled);
    }

    /**
     * @brief Enable or Disable the insertion of file-related timestamps in
     *        the simulation output (enabled by default)
     * @param enabled true to enable, false to disable
     */
    void SimulationOutput::enableFileReadWriteCopyTimestamps(bool enabled) {
        this->setEnabled<SimulationTimestampFileReadStart>(enabled);
        this->setEnabled<SimulationTimestampFileReadFailure>(enabled);
        this->setEnabled<SimulationTimestampFileReadCompletion>(enabled);
        this->setEnabled<SimulationTimestampFileWriteStart>(enabled);
        this->setEnabled<SimulationTimestampFileWriteFailure>(enabled);
        this->setEnabled<SimulationTimestampFileWriteCompletion>(enabled);
        this->setEnabled<SimulationTimestampFileCopyStart>(enabled);
        this->setEnabled<SimulationTimestampFileCopyFailure>(enabled);
        this->setEnabled<SimulationTimestampFileCopyCompletion>(enabled);
    }

    /**
     * @brief Enable or Disable the insertion of energy-related timestamps in
     *        the simulation output (enabled by default)
     * @param enabled true to enable, false to disable
     */
    void SimulationOutput::enableEnergyTimestamps(bool enabled) {
        this->setEnabled<SimulationTimestampPstateSet>(true);
        this->setEnabled<SimulationTimestampEnergyConsumption>(true);
    }

    /**
     * @brief Enable or Disable the insertion of link-usage-related timestamps in
     *        the simulation output (enabled by default)
     * @param enabled true to enable, false to disable
     */
    void SimulationOutput::enableBandwidthTimestamps(bool enabled) {
        this->setEnabled<SimulationTimestampLinkUsage>(true);
    }



};

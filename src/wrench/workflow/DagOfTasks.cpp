/**
 * Copyright (c) 2017-2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <vector>
#include <wrench/workflow/DagOfTasks.h>
#include <wrench/logging/TerminalOutput.h>
#include <boost/property_map/property_map.hpp>


WRENCH_LOG_CATEGORY(dag_of_tasks, "Log category for DagOfTasks");

namespace wrench {

    /**
 * @brief Method to add a task vertex to the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::addVertex(const wrench::WorkflowTask *task) {
        // Add a new vertex
        VertexProperties p = {task};
        boost::add_vertex(p, this->dag);
        // Update the vertex vector
        this->task_list.push_back(task);
        // Set the task's vertex id in the task map
        this->task_map[task] = this->task_list.size() - 1;
    }

    /**
 * @brief Method to remove a task vertex from the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::removeVertex(wrench::WorkflowTask *task) {
        // Find the vertex
        if (this->task_map[task] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying to remove a non-existing vertex");
        }
        // Remove the correspond task list item
        this->task_list.erase(this->task_list.begin() + this->task_map[task]);
        // Update subsequent task's vertex indices
        for (auto it = this->task_list.begin() + this->task_map[task]; it != this->task_list.end(); ++it) {
            this->task_map[*it]--;
        }

        // Remove all in and out edges at that vertex
        boost::clear_vertex(this->task_map[task], this->dag);

        // Remove the vertex
        boost::remove_vertex(this->task_map[task], this->dag);
    }


    /**
 * @brief Method to add an edge between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::addEdge(wrench::WorkflowTask *src, wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (this->task_map[src] >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::removeVertex(): Trying to add an edge from a non-existing vertex");
        }
        if (this->task_map[dst] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying to add an edge to a non-existing vertex");
        }

        // Add the edge
        boost::add_edge(this->task_map[src], this->task_map[dst], this->dag);
    }

    /**
 * @brief Remove an edge between two task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::removeEdge(wrench::WorkflowTask *src, wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (this->task_map[src] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge from a non-existing vertex");
        }
        if (this->task_map[dst] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge to a non-existing vertex");
        }

        // Remove the edge
        boost::remove_edge(this->task_map[src], this->task_map[dst], this->dag);
    }

    /**
 * @brief Method to check whether a path exists between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 * @return true if there is a path between the tasks
 */
    bool wrench::DagOfTasks::doesPathExist(const wrench::WorkflowTask *src, const wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (this->task_map[src] >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path from a non-existing vertex");
        }
        if (this->task_map[dst] >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path to a non-existing vertex");
        }
        // Find the vertices
        auto src_vertex = this->task_map[src];
        auto dst_vertex = this->task_map[dst];

        auto indexmap = boost::get(boost::vertex_index, dag);
        auto colormap = boost::make_vector_property_map<boost::default_color_type>(indexmap);

        using vertex_descriptor = boost::graph_traits<DAG>::vertex_descriptor;
        boost::queue<vertex_descriptor> Q;
        custom_bfs_visitor vis(dst);

        try {
            boost::breadth_first_visit(dag, src_vertex, Q, vis, colormap);
        } catch (std::runtime_error &e) {
            return true;
        }
        return false;
    }

    /**
 * @brief Method to check whether an edge exists between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 * @return true if there is a path between the tasks
 */
    bool wrench::DagOfTasks::doesEdgeExist(const wrench::WorkflowTask *src, const wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (this->task_map[src] >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path from a non-existing vertex");
        }
        if (this->task_map[dst] >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path to a non-existing vertex");
        }
        // Find the vertices
        auto src_vertex = this->task_map[src];
        auto dst_vertex = this->task_map[dst];

        return boost::edge(src_vertex, dst_vertex, this->dag).second;
    }

    /**
 * @brief Method to get the number of children of a task vertex
 * @param task: the task
 * @return a number children
 */
    long wrench::DagOfTasks::getNumberOfChildren(const WorkflowTask *task) {
        // Find the vertex
        if (this->task_map[task] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfChildren(): Non-existing vertex");
        }
        auto vertex = this->task_map[task];
        boost::graph_traits<DAG>::out_edge_iterator eo, edge_end;
        long count = 0;
        for (boost::tie(eo, edge_end) = boost::out_edges(vertex, dag); eo != edge_end; ++eo) {
            count++;
        }
        return count;
    }

    /**
 * @brief Method to get the children of a task vertex
 * @param task: the task
 * @return the children
 */
    std::vector<WorkflowTask *> wrench::DagOfTasks::getChildren(const WorkflowTask *task) {
        // Find the vertex
        if (this->task_map[task] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getChildren(): Non-existing vertex");
        }
        auto vertex = this->task_map[task];
        boost::graph_traits<DAG>::out_edge_iterator eo, edge_end;
        std::vector<WorkflowTask *> children;
        for (boost::tie(eo, edge_end) = boost::out_edges(vertex, dag); eo != edge_end; ++eo) {
            // Discard the const qualifier
            children.push_back((WorkflowTask *) (dag[target(*eo, dag)].task));
        }
        return children;
    }

    /**
 * @brief Method to get the number of parents of a task vertex
 * @param task: the task
 * @return a number parents
 */
    long wrench::DagOfTasks::getNumberOfParents(const WorkflowTask *task) {
        // Find the vertex
        if (this->task_map[task] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfParents(): Non-existing vertex");
        }
        auto vertex = this->task_map[task];
        boost::graph_traits<DAG>::in_edge_iterator ei, edge_end;
        long count = 0;
        for (boost::tie(ei, edge_end) = boost::in_edges(vertex, dag); ei != edge_end; ++ei) {
            count++;
        }
        return count;
    }

    /**
     * @brief Method to get the parents of a task vertex
     * @param task: the task
     * @return the parents
     */
    std::vector<WorkflowTask *> wrench::DagOfTasks::getParents(const WorkflowTask *task) {
        // Find the vertex
        if (this->task_map[task] >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getParents(): Non-existing vertex");
        }
        auto vertex = this->task_map[task];
        boost::graph_traits<DAG>::in_edge_iterator ei, edge_end;
        std::vector<WorkflowTask *> parents;
        for (boost::tie(ei, edge_end) = boost::in_edges(vertex, dag); ei != edge_end; ++ei) {
            // Discard the const qualifier
            parents.push_back((WorkflowTask *) (dag[source(*ei, dag)].task));
        }
        return parents;
    }

}// namespace wrench
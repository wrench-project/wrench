/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <vector>
#include "wrench/workflow/DagOfTasks.h"
#include "wrench/logging/TerminalOutput.h"
#include <boost/property_map/property_map.hpp>


WRENCH_LOG_CATEGORY(dag_of_tasks, "Log category for DagOfTasks");

namespace wrench {

/**
 * @brief Method to add a task vertex to the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::addVertex( wrench::WorkflowTask *task) {

        // Add a new vertex
        VertexProperties p = {task};
        boost::add_vertex(p, this->dag);
        // Update the vertex vector
        this->task_list.push_back(task);
        // Set the task's vertex id
        task->dag_vertex_index = this->task_list.size() - 1;
    }

/**
 * @brief Method to remove a task vertex from the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::removeVertex(wrench::WorkflowTask *task) {
        // Find the vertex
        if (task->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying to remove a non-existing vertex");
        }
        // Remove the correspond task list item
        this->task_list.erase(this->task_list.begin() + task->dag_vertex_index);
        // Update subsequence task's vertex indices
        for (auto it = this->task_list.begin()  +  task->dag_vertex_index; it != this->task_list.end(); ++it) {
            (*it)->dag_vertex_index--;
        }

        // Remove all in and out edges at that vertex
        boost::clear_vertex(task->dag_vertex_index, this->dag);

        // Remove the vertex
        boost::remove_vertex(task->dag_vertex_index, this->dag);

        // Set the task's vertex index to infinity, just in case
        task->dag_vertex_index = ULONG_MAX;

    }


/**
 * @brief Method to add an edge between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::addEdge( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (src->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::removeVertex(): Trying to add an edge from a non-existing vertex");
        }
        if (dst->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying to add an edge to a non-existing vertex");
        }

        // Add the edge
        boost::add_edge(src->dag_vertex_index, dst->dag_vertex_index, this->dag);
    }

/**
 * @brief Remove an edge between two task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::removeEdge( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (src->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge from a non-existing vertex");
        }
        if (dst->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge to a non-existing vertex");
        }

        // Remove the edge
        boost::remove_edge(src->dag_vertex_index, src->dag_vertex_index, this->dag);
    }

/**
 * @brief Method to check whether a path exists between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 * @return trus if there is a path netween the tasks
 */
    bool wrench::DagOfTasks::doesPathExist( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (src->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path from a non-existing vertex");
        }
        if (src->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path to a non-existing vertex");
        }
        // Find the vertices
        auto src_vertex = src->dag_vertex_index;
        auto dst_vertex = src->dag_vertex_index;

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
 * @brief Method to get the number of children of a task vertex
 * @param task: the task
 * @return a number children
 */
    long wrench::DagOfTasks::getNumberOfChildren(const WorkflowTask *task) {
        // Find the vertex
        if (task->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfChildren(): Non-existing vertex");
        }
        auto vertex = task->dag_vertex_index;
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
    std::vector< WorkflowTask *> wrench::DagOfTasks::getChildren(const WorkflowTask *task) {
        // Find the vertex
        if (task->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getChildren(): Non-existing vertex");
        }
        auto vertex = task->dag_vertex_index;
        boost::graph_traits<DAG>::out_edge_iterator eo, edge_end;
        std::vector< WorkflowTask *> children;
        for (boost::tie(eo, edge_end) = boost::out_edges(vertex, dag); eo != edge_end; ++eo) {
            children.push_back(dag[target(*eo, dag)].task);
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
        if (task->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfParents(): Non-existing vertex");
        }
        auto vertex = task->dag_vertex_index;
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
    std::vector< WorkflowTask *> wrench::DagOfTasks::getParents(const WorkflowTask *task) {
        // Find the vertex
        if (task->dag_vertex_index >= this->task_list.size()) {
            throw std::runtime_error("wrench::DagOfTasks::getParents(): Non-existing vertex");
        }
        auto vertex = task->dag_vertex_index;
        boost::graph_traits<DAG>::in_edge_iterator ei, edge_end;
        std::vector< WorkflowTask *> parents;
        for (boost::tie(ei, edge_end) = boost::in_edges(vertex, dag); ei != edge_end; ++ei) {
            parents.push_back(dag[source(*ei, dag)].task);
        }
        return parents;
    }

}
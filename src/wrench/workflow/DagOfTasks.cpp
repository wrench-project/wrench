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

namespace wrench {

/**
 * @brief Method to add a task vertex to the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::addVertex( wrench::WorkflowTask *task) {

        // Add a new vertex
        vertex_t new_vertex = boost::add_vertex(this->dag);
        // Associate the task to it
        this->dag[new_vertex].task = task;
        // Add the pair to the taskmap
        this->task_map[task] = new_vertex;
    }

/**
 * @brief Method to remove a task vertex from the DAG
 * @param task: the task
 */
    void wrench::DagOfTasks::removeVertex( wrench::WorkflowTask *task) {
        // Find the vertex
        if (task_map.find(task) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying to remove a non-existing vertex");
        }
        vertex_t to_remove = task_map[task];

        // Remove all in and out edges at that vertex
        boost::clear_vertex(to_remove, this->dag);

        // Remove the entry from the task map
        this->task_map.erase(task);
        // Remove the vertex
        boost::remove_vertex(to_remove, this->dag);
    }

/**
 * @brief Method to add an edge between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::addEdge( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (task_map.find(src) == task_map.end()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::removeVertex(): Trying add an edge from a non-existing vertex");
        }
        if (task_map.find(dst) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::removeVertex(): Trying add an edge to a non-existing vertex");
        }
        // Find the endpoints
        vertex_t src_vertex = task_map[src];
        vertex_t dst_vertex = task_map[dst];
        // Add the edge
        boost::add_edge(src_vertex, dst_vertex, this->dag);
    }

/**
 * @brief Remove an edge between two task vertices
 * @param src: the source task
 * @param dst: the destination task
 */
    void wrench::DagOfTasks::removeEdge( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (task_map.find(src) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge from a non-existing vertex");
        }
        if (task_map.find(dst) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::removeEdge(): Trying add an edge to a non-existing vertex");
        }
        // Find the vertices
        vertex_t src_vertex = task_map[src];
        vertex_t dst_vertex = task_map[dst];
        // Remove the edge
        boost::remove_edge(src_vertex, dst_vertex, this->dag);
    }

/**
 * @brief Method to check whether a path exists between to task vertices
 * @param src: the source task
 * @param dst: the destination task
 * @return trus if there is a path netween the tasks
 */
    bool wrench::DagOfTasks::doesPathExist( wrench::WorkflowTask *src,  wrench::WorkflowTask *dst) {
        // Check that vertices exist
        if (task_map.find(src) == task_map.end()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path from a non-existing vertex");
        }
        if (task_map.find(dst) == task_map.end()) {
            throw std::runtime_error(
                    "wrench::DagOfTasks::doesPathExist(): Trying to find a path to a non-existing vertex");
        }
        // Find the vertices
        vertex_t src_vertex = task_map[src];
        vertex_t dst_vertex = task_map[dst];

        auto indexmap = boost::get(boost::vertex_index, dag);
        auto colormap = boost::make_vector_property_map<boost::default_color_type>(indexmap);

        using vertex_descriptor = boost::graph_traits<DAG>::vertex_descriptor;
        boost::queue<vertex_descriptor> Q;
        custom_bfs_visitor vis(dst_vertex);

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
        if (task_map.find(task) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfChildren(): Non-existing vertex");
        }
        vertex_t vertex = task_map[task];
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
        if (task_map.find(task) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::getChildren(): Non-existing vertex");
        }
        vertex_t vertex = task_map[task];
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
        if (task_map.find(task) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::getNumberOfParents(): Non-existing vertex");
        }
        vertex_t vertex = task_map[task];
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
        if (task_map.find(task) == task_map.end()) {
            throw std::runtime_error("wrench::DagOfTasks::getParents(): Non-existing vertex");
        }
        vertex_t vertex = task_map[task];
        boost::graph_traits<DAG>::in_edge_iterator ei, edge_end;
        std::vector< WorkflowTask *> parents;
        for (boost::tie(ei, edge_end) = boost::in_edges(vertex, dag); ei != edge_end; ++ei) {
            parents.push_back(dag[target(*ei, dag)].task);
        }
        return parents;
    }

}
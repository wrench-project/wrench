/**
 * Copyright (c) 2017-2019. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_DAGOFTASKS_H
#define WRENCH_DAGOFTASKS_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <iostream>

#include <wrench/workflow/WorkflowTask.h>

namespace wrench {

/***********************/
/** \cond INTERNAL     */
/***********************/

    class WorkflowTask;

    /**
     * @brief Data structure to store vertex properties
     */
    struct VertexProperties {
//    std::size_t index;
//    boost::default_color_type color;
        /** @brief Task attached to the vertex */
        const WorkflowTask *task;
    };

    /**
     * @brief Convenient DAG typedef
     */
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, VertexProperties> DAG;

    /**
     * @brief Convenient vertext_t typedef
     */
    typedef unsigned long vertex_t;  // To clean up some day...

    /**
     * @brief An internal class that uses the Boost Graph Library to implement a DAG of WorkflowTask objects
     */
    class DagOfTasks {

    public:

        void addVertex(const WorkflowTask *task);

        void removeVertex(WorkflowTask *task);

        void addEdge(WorkflowTask *src, WorkflowTask *dst);

        void removeEdge(WorkflowTask *src, WorkflowTask *dst);

        bool doesPathExist(const WorkflowTask *src, const WorkflowTask *dst);
        bool doesEdgeExist(const WorkflowTask *src, const WorkflowTask *dst);

        long getNumberOfChildren(const WorkflowTask *task);

        std::vector<WorkflowTask *> getChildren(const WorkflowTask *task);

        long getNumberOfParents(const WorkflowTask *task);

        std::vector<WorkflowTask *> getParents(const WorkflowTask *task);

    private:

        /**
         * @brief Nested class that's used for the BFS algorithm in the BGL
         */
        class custom_bfs_visitor : public boost::default_bfs_visitor {
        public:

            const WorkflowTask *target_task;

            explicit custom_bfs_visitor(const WorkflowTask *target_task) : boost::default_bfs_visitor() {
                this->target_task = target_task;
            }

            template<typename Vertex, typename Graph>
            void discover_vertex(Vertex u,  Graph &g) {
                if (g[u].task == target_task) {
                    throw std::runtime_error("path found");
                }
            }
        };

        std::vector<const WorkflowTask*> task_list;
        std::unordered_map<const WorkflowTask *, unsigned long> task_map;

        DAG dag;

    };

/***********************/
/** \endcond           */
/***********************/

}


#endif //WRENCH_DAGOFTASKS_H

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

    struct Vertex {
         WorkflowTask *task;
    };
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::bidirectionalS, Vertex> DAG;
    typedef boost::graph_traits<DAG>::vertex_descriptor vertex_t;

    /**
     * @brief An internal class that uses the Boost Graph Library to implement a DAG of WorkflowTask objects
     */
    class DagOfTasks {

    public:

        void addVertex(WorkflowTask *task);

        void removeVertex(WorkflowTask *task);

        void addEdge(WorkflowTask *src, WorkflowTask *dst);

        void removeEdge(WorkflowTask *src, WorkflowTask *dst);

        bool doesPathExist(WorkflowTask *src, WorkflowTask *dst);

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

            vertex_t target;

            explicit custom_bfs_visitor(vertex_t a) : boost::default_bfs_visitor() {
                this->target = a;
            }

            template<typename Vertex, typename Graph>
            void discover_vertex(Vertex u,  Graph &g) {
                std::cerr << "Visiting " << g[u].task->getID() << " " << g[target].task->getID() << std::endl;
                if (g[u].task == g[target].task) {
                    throw std::runtime_error("found it!");
                }
            }
        };

        std::unordered_map<const WorkflowTask *, vertex_t> task_map;
        DAG dag;

    };

/***********************/
/** \endcond           */
/***********************/


}


#endif //WRENCH_DAGOFTASKS_H

//
// Created by james oeth on 9/22/17.
//

#include <lemon/list_graph.h>
#include <lemon/graph_to_eps.h>
#include <pugixml.hpp>
#include <json.hpp>



#include "wrench/util/WorkflowUtil.h"

namespace wrench {
    namespace WorkflowUtil {
        void loadFromDAX(const std::string &filename, Workflow *workflow) {
            pugi::xml_document dax_tree;

            if (not dax_tree.load_file(filename.c_str())) {
                throw std::invalid_argument("WorkflowUtil::loadFromDAX(): Invalid DAX file");
            }

            // Get the root node
            pugi::xml_node dag = dax_tree.child("adag");

            // Iterate through the "job" nodes
            for (pugi::xml_node job = dag.child("job"); job; job = job.next_sibling("job")) {
                wrench::WorkflowTask *task;
                // Get the job attributes
                std::string id = job.attribute("id").value();
                std::string name = job.attribute("name").value();
                double flops = std::strtod(job.attribute("runtime").value(), NULL);
                int num_procs = 1;
                if (job.attribute("num_procs")) {
                    num_procs = std::stoi(job.attribute("num_procs").value());
                }
                // Create the task
                //task = this->addTask(id + "_" + name, flops, num_procs);
                task = workflow->addTask(id + "_" + name, flops, num_procs);

                // Go through the children "uses" nodes
                for (pugi::xml_node uses = job.child("uses"); uses; uses = uses.next_sibling("uses")) {
                    // getMessage the "uses" attributes
                    // TODO: There are several attributes that we're ignoring for now...
                    std::string id = uses.attribute("file").value();

                    double size = std::strtod(uses.attribute("size").value(), NULL);
                    std::string link = uses.attribute("link").value();
                    // Check whether the file already exists
                    wrench::WorkflowFile *file = nullptr;

                    try {
                        file = workflow->getWorkflowFileByID(id);
                    } catch (std::invalid_argument) {
                        file = workflow->addFile(id, size);
                    }
                    if (link == "input") {
                        task->addInputFile(file);
                    }
                    if (link == "output") {
                        task->addOutputFile(file);
                    }
                    // TODO: Are there other types of "link" values?
                }
            }
        }

        void loadFromJson(const std::string &filename, Workflow *workflow) {
            ///make workflow task
            wrench::WorkflowTask *task;

            /// read a JSON file
            std::ifstream file;
            nlohmann::json j;
            std::cerr << "parsing json" << std::endl;

            //handle the exceptions of opening the json file
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try {
                file.open(filename);
                file >> j;
            }
            catch (const std::ifstream::failure &e) {
                std::cerr << "Exception opening/reading file";
                throw std::invalid_argument("WorkflowUtil::loadFromJson(): Invalid Json file");

            }
            nlohmann::json workflowJobs;
            try {
                workflowJobs = j.at("workflow");
            }
            catch (std::out_of_range) {
                std::cerr << "out of range" << '\n';
            }
//            std::cerr << "\n" << "outputting jobs" << std::endl;
            for (nlohmann::json::iterator it = workflowJobs.begin(); it != workflowJobs.end(); ++it) {
//                std::cerr << "jobs outputting hopefully" << std::endl;
                if (it.key() == "jobs") {
                    std::vector<nlohmann::json> jobs = it.value();
                    for (unsigned int i = 0; i < jobs.size(); i++) {
//                            std::cerr << jobs[i].at("name") << std::endl;
                        std::string name = jobs[i].at("name");
                        double flops = jobs[i].at("runtime");
//                            std::cerr << jobs[i].at("runtime") << std::endl;
                        int num_procs = 1;
                        task = workflow->addTask(name, flops, num_procs);
                        std::vector<nlohmann::json> files = jobs[i].at("files");
//                        std::cerr << "files   " << files[0] << std::endl;
                        for (unsigned int j = 0; j < files.size(); j++) {
                            double size = files[j].at("size");
                            std::string link = files[j].at("link");
                            std::string id = files[j].at("name");
//                            std::cerr << "name:  " << id << "  Link  " << link << "  size  " << size << std::endl;
                            wrench::WorkflowFile *file = nullptr;
                            try {
                                file = workflow->getWorkflowFileByID(id);
                            } catch (const std::invalid_argument &ia) {
//                                std::cerr << "making a new file bc  " <<  ia.what()  << std::endl;
                                file = workflow->addFile(id, size);
                                if (link == "input") {
                                    task->addInputFile(file);
                                }
                                if (link == "output") {
                                    task->addOutputFile(file);
                                }
                            }

                        }
                    }
                }
            }
            file.close();
        }

    }
}

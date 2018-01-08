//
// Created by Suraj Pandey on 12/26/17.
//

#include <fstream>
#include "wrench/util/TraceFileLoader.h"

namespace wrench {
        /**
        * @brief Load the workflow trace file
        *
        * @param filename: the path to the trace file
        *
        * @throw std::invalid_argument
        */

        std::vector<std::pair<double,std::tuple<std::string, double, int, int, double, int>>> TraceFileLoader::loadFromTraceFile(std::string filename,double load_time_compensation) {
            std::vector<std::pair<double,std::tuple<std::string, double, int, int, double, int>>> trace_file_jobs = {};
            try {
                std::ifstream infile(filename);
                infile.exceptions(std::ifstream::failbit);
                std::string line;
                while (std::getline(infile, line))
                {
                    if(line[0]==';'){

                    }else{
                        std::istringstream iss(line);
                        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                                        std::istream_iterator<std::string>{}};
                        std::string id;
                        double flops, parallel_efficiency;
                        int min_num_cores, max_num_cores;
                        int itemnum = 0;
                        double sub_time = 0;
                        int num_nodes = 0;
                        for(auto item:tokens){
                            switch (itemnum){
                                case 0:
                                    id = item;
                                    break;
                                case 1:
                                    if (sscanf(item.c_str(), "%lf", &sub_time) != 1) {
                                        throw std::invalid_argument(
                                                "TraceFileLoader::loadFromTraceFile(): Invalid submission time in trace file '" + item + "'");
                                    }
                                    sub_time+=load_time_compensation;
                                    break;
                                case 2:
                                    //probably not required right now (this one is wait time)
                                    break;
                                case 3:
                                    //assuming flops and runtime are the same (in seconds)
                                    if (sscanf(item.c_str(), "%lf", &flops) != 1) {
                                        throw std::invalid_argument(
                                                "TraceFileLoader::loadFromTraceFile(): Invalid submission time in trace file '" + item + "'");
                                    }
                                    break;
                                case 4:
                                    //min and max cores making the same, the total cores in the node
                                    //assuming flops and runtime are the same (in seconds)
                                    if (sscanf(item.c_str(), "%d", &num_nodes) != 1) {
                                        throw std::invalid_argument(
                                                "TraceFileLoader::loadFromTraceFile(): Invalid submission time in trace file '" + item + "'");
                                    }
                                    max_num_cores = -1;
                                    min_num_cores = -1;
                                    break;
                                case 5:
                                case 6:
                                case 7:
                                case 8:
                                case 9:
                                case 10:
                                    // Average CPU Time Used, Used Memory, Requested Number of Processors, Requested Time, Requested Memory, Status
                                    // These all fields are probably meaningless in models, only relevant in real logs
                                    break;
                                case 11:
                                    //This is used id, but we don't have/need this feature yet
                                    break;
                                case 12:
                                    //This is group id, but we don't have/need this yet
                                    break;
                                case 13:
                                    // Executable (Application number), probably not necessary
                                    break;
                                case 14:
                                    // Queue number, is maintained inside the simulation, (we have only one queue)
                                    break;
                                case 15:
                                case 16:
                                case 17:
                                    // Partition Number, Preceeding Job number, Think time from Preceeding Job
                                    // Not necessary in case of models, I guess
                                    break;
                                default:
                                    throw std::runtime_error(
                                            "TraceFileLoader::loadFromTraceFile(): Unknown trace file column, may be there are more than 18 columns in trace file"
                                    );
                            }
                            itemnum++;
                        }
                        std::pair<double,std::tuple<std::string, double, int, int, double, int>> task = std::make_pair(sub_time,std::tuple<std::string, double, int, int, double, int>(
                                id, flops, min_num_cores, max_num_cores, 1, num_nodes
                        ));
                        trace_file_jobs.push_back(task);
                    }
                }
            }catch (std::exception e){
                std::cout<<"Got an exception: "<<e.what()<<"\n";
            }
            return trace_file_jobs;
        }
}
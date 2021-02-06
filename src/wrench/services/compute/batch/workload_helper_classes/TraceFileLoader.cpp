/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <fstream>
#include "wrench/logging/TerminalOutput.h"
#include <wrench-dev.h>
#include <nlohmann/json.hpp>
#include "wrench/util/TraceFileLoader.h"

WRENCH_LOG_CATEGORY(wrench_core_trace_file_loader, "Log category for Trace File Loader");


namespace wrench {

    /**
     * @brief A method to generate a random username, so that generated workload
     * traces look more realistic
     *
     * @param userid: numerical userid
     * @return a generated alpha userid
     */
    std::string generateRandomUsername(unsigned long userid) {
        //Type of random number distribution
        const char charset[] =
                "aabccdeeefghijklmnooopqrstttuuvwxyzz";
        std::uniform_int_distribution<int> dist(0, sizeof(charset)-2);
        //Mersenne Twister: Good quality random number generator
        std::mt19937 rng;
        rng.seed(userid);  // Consistent for the same userid
        std::string username = "";
        int username_length = 3 + dist(rng) % 5;
        while(username_length--) {
            username += charset[dist(rng)];
        }
        return username;
    }


    /**
    * @brief Load the workflow trace file
    *
    * @param filename: the path to the trace file in SWF format or in JSON format
    * @param ignore_invalid_jobs: whether to ignore invalid job specifications
    * @param desired_submit_time_of_first_job: the desired submit of of the first job (-1 means "use whatever time is in the trace file")
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *              - username
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>>
    TraceFileLoader::loadFromTraceFile(std::string filename, bool ignore_invalid_jobs, double desired_submit_time_of_first_job) {

        std::istringstream ss(filename);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, '.')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 2) {
            throw std::invalid_argument(
                    "TraceFileLoader::loadFromTraceFile(): batch workload trace file name must end with '.swf' or '.json'");
        }
        std::string extension = tokens[tokens.size() - 1];
        if (extension == "swf") {
            return loadFromTraceFileSWF(filename, ignore_invalid_jobs, desired_submit_time_of_first_job);
        } else if (extension == "json") {
            return loadFromTraceFileJSON(filename, ignore_invalid_jobs, desired_submit_time_of_first_job);
        } else {
            throw std::invalid_argument(
                    "TraceFileLoader::loadFromTraceFile(): batch workload trace file name must end with '.swf' or '.json'");
        }
    }

    /**
    * @brief Load the workflow SWF trace file
    *
    * @param filename: the path to the trace file in SWF format
    * @param ignore_invalid_jobs: whether to ignore invalid job specifications
    * @param desired_submit_time_of_first_job: the desired submit of of the first job (-1 means "use whatever time is in the trace file")
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *              - username
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>>
    TraceFileLoader::loadFromTraceFileSWF(std::string filename, bool ignore_invalid_jobs, double desired_submit_time_of_first_job) {

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>> trace_file_jobs = {};

        std::ifstream infile(filename);
        if (not infile.is_open()) {
            throw std::invalid_argument(
                    "TraceFileLoader::loadFromTraceFileSWF(): Cannot open batch workload trace file " + filename);
        }


        std::string line;
        double original_submit_time_of_first_job = -1;
        while (std::getline(infile, line)) {

            if (line[0] != ';') {
                std::istringstream iss(line);
                std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                                std::istream_iterator<std::string>{}};

                std::string id;
                double time = -1, requested_time = -1, requested_ram = -1;
                int itemnum = 0;
                double sub_time = -1;
                int requested_num_nodes = -1;
                int num_nodes = -1;
                unsigned long userid = 0;
                std::string username;

                try {
                    if (tokens.size() < 10) {
                        throw std::invalid_argument(
                                "TraceFileLoader::loadFromTraceFileSWF(): Seeing less than 10 fields per line in batch workload trace file '" +
                                filename +
                                "'");
                    }

                    for (auto const &item : tokens) {
                        switch (itemnum) {
                            case 0: // Job ID
                                id = item;
                                break;
                            case 1: // Submit time
                                if (sscanf(item.c_str(), "%lf", &sub_time) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid submission time '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                if (original_submit_time_of_first_job < 0) {
                                    original_submit_time_of_first_job = sub_time;
                                }
                                if (desired_submit_time_of_first_job >= 0) {
                                    sub_time += (desired_submit_time_of_first_job - original_submit_time_of_first_job);
                                }
                                break;
                            case 2: // Wait time
                                break;
                            case 3: // Run time
                                //assuming flops and runtime are the same (in seconds)
                                if (sscanf(item.c_str(), "%lf", &time) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid run time '" + item +
                                            "' in batch workload trace file");
                                }
                                break;
                            case 4: // Number of Allocated Processors
                                if (sscanf(item.c_str(), "%d", &num_nodes) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid number of processors '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                break;
                            case 5:// Average CPU time Used
                                break;
                            case 6: // Used Memory
                                break;
                            case 7: // Requested Number of Processors
                                if (sscanf(item.c_str(), "%d", &requested_num_nodes) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested number of processors '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                break;
                            case 8: // Requested time
                                //assuming flops and runtime are the same (in seconds)
                                if (sscanf(item.c_str(), "%lf", &requested_time) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested time '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                break;
                            case 9: // Requested memory_manager_service
                                // In KiB
                                if (sscanf(item.c_str(), "%lf", &requested_ram) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested memory_manager_service '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                requested_ram *= 1024.0;
                                break;
                            case 10: // Status
                                break;
                            case 11: // User ID
                                if (sscanf(item.c_str(), "%lu", &userid) != 1) {
                                    throw std::invalid_argument(
                                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid userid '" +
                                            item +
                                            "' in batch workload trace file");
                                }
                                break;
                            case 12: // Group ID
                                break;
                            case 13: // Executable number
                                break;
                            case 14: // Queue number
                                break;
                            case 15: // Partition number
                                break;
                            case 16: // Preceding job number
                                break;
                            case 17: // Think time
                                break;
                            default:
                                throw std::invalid_argument(
                                        "TraceFileLoader::loadFromTraceFileSWF(): Unknown batch workload trace file column, maybe there are more than 18 columns?"
                                );
                        }
                        itemnum++;
                    }

                    // Fix/check values
                    if (requested_time < 0) {
                        requested_time = time;
                    } else if (time < 0) {
                        time = requested_time;
                    }
                    if ((requested_time < 0) or (time < 0)) {
                        throw std::invalid_argument(
                                "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative flops (" +
                                std::to_string(time) + ") and negative requested flops ("+ std::to_string(requested_time) + ") in batch workload trace file");
                    }
                    if (requested_time < time) {
                        WRENCH_WARN(
                                "TraceFileLoader::loadFromTraceFileSWF(): invalid job with requested time (%lf) smaller than actual time (%lf) in batch workload trace file [fixing it]", requested_time, time);
                        requested_time = time;
                    }

                    if (requested_ram < 0) {
                        requested_ram = 0;
                    }

                    if (sub_time < 0) {
                        throw std::invalid_argument(
                                "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative submission time in batch workload trace file");
                    }
                    if (requested_num_nodes <= 0) {
                        requested_num_nodes = num_nodes;
                    }
                    if (requested_num_nodes <= 0) {
                        throw std::invalid_argument(
                                "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative (requested) number of node in batch workload trace file");
                    }
                    if (userid == 0) {
                        username = "user";
                    } else {
                        username = generateRandomUsername(userid);
                    }
                } catch (std::invalid_argument &e) {
                    if (ignore_invalid_jobs) {
                        WRENCH_WARN("%s (in batch workload file %s) IGNORING", e.what(), filename.c_str());
                        continue;
                    } else {
                        throw std::invalid_argument("Error while reading batch workload trace file " + filename + ": " +  e.what());
                    }
                }

                // Add the job to the list
                std::tuple<std::string, double, double, double, double, unsigned int, std::string> job =
                        std::tuple<std::string, double, double, double, double, unsigned int, std::string>(
                                username, sub_time, time, requested_time, requested_ram,
                                (unsigned int) requested_num_nodes, username);
                trace_file_jobs.push_back(job);
            }
        }
        return trace_file_jobs;
    }

    /**
    * @brief Load the workflow JSON trace file
    *
    * @param filename: the path to the trace file in JSON format
    * @param ignore_invalid_jobs: whether to ignore invalid job specifications
    * @param desired_submit_time_of_first_job: the desired submit of of the first job (-1 means "use whatever time is in the trace file")
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *              - username
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>>
    TraceFileLoader::loadFromTraceFileJSON(std::string filename, bool ignore_invalid_jobs, double desired_submit_time_of_first_job) {

        std::vector<std::tuple<std::string, double, double, double, double, unsigned int, std::string>> trace_file_jobs = {};

        std::ifstream file;
        nlohmann::json j;


        //handle the exceptions of opening the json file
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            file.open(filename);
        } catch (const std::ifstream::failure &e) {
            throw std::invalid_argument(
                    "TraceFileLoader::loadFromTraceFileJSON(): Cannot open JSON batch workload trace file " + filename);
        }

        try {
            file >> j;
            file.close();
        } catch (const std::exception &e) {
            throw std::invalid_argument(
                    std::string("TraceFileLoader::loadFromTraceFileJSON(): JSON parse error: ") + e.what());
        }

        nlohmann::json jobs;
        try {
            jobs = j.at("jobs");
        } catch (std::exception &e) {
            throw std::invalid_argument(
                    "TraceFileLoader::loadFromTraceFileJSON(): Could not find 'jobs' in JSON batch workload trace file");
        }

                double original_submit_time_of_first_job = -1;

        for (nlohmann::json::iterator it = jobs.begin(); it != jobs.end(); ++it) {
            nlohmann::json json_job = it.value();
            unsigned id;
            unsigned long res;
            double subtime;
            double walltime;
            double requested_time;

            try {
                if (not json_job.is_object()) {
                    // Broken JSON
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileJSON(): broken job specification in JSON batch workload trace file");
                }


                try {
                    id = json_job.at("id");
                    res = json_job.at("res");
                    subtime = json_job.at("subtime");
                    if (original_submit_time_of_first_job < 0) {
                        original_submit_time_of_first_job = subtime;
                    }
                    walltime = json_job.at("walltime");
                } catch (std::exception &e) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileJSON(): invalid job specification in JSON batch workload trace file");
                }

                // Fix/check values
                if ((res <= 0) or (subtime < 0) or (walltime < 0)) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileJSON(): invalid job specification in JSON batch workload trace file");
                }
                if (desired_submit_time_of_first_job >= 0) {
                    subtime += (desired_submit_time_of_first_job - original_submit_time_of_first_job);
                }

                // It seems the Batsim JSON format does not include requested
                // runtimes, and so we set the requested runtime to the walltime
                // (i.e., users always ask exactly for what they need)
                requested_time = walltime;
            } catch (std::invalid_argument &e) {
                if (ignore_invalid_jobs) {
                    WRENCH_WARN("%s (in batch workload file %s)", e.what(), filename.c_str());
                    continue;
                } else {
                    throw std::invalid_argument("Error while reading batch workload trace file " + filename + ": " +  e.what());
                }
            }

//      // Add the job to the list
            std::tuple<std::string, double, double, double, double, unsigned int, std::string> job =
                    std::tuple<std::string, double, double, double, double, unsigned int, std::string>(
                            std::to_string(id), subtime, walltime, requested_time, 0.0, (unsigned int) res, "user");
            trace_file_jobs.push_back(job);
        }

        return trace_file_jobs;
    }
}

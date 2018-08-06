/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <fstream>
#include <xbt/log.h>
#include <wrench-dev.h>
#include <nlohmann/json.hpp>
#include "wrench/util/TraceFileLoader.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(trace_file_loader, "Log category for Trace File Loader");

namespace wrench {

    /**
    * @brief Load the workflow trace file
    *
    * @param filename: the path to the trace file in SWF format or in JSON format
    * @param load_time_compensation: an offset to add to submit times in the trace file
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int>>
    TraceFileLoader::loadFromTraceFile(std::string filename, double load_time_compensation) {

      std::istringstream ss(filename);
      std::string token;
      std::vector<std::string> tokens;

      while (std::getline(ss, token, '.')) {
        tokens.push_back(token);
      }

      if (tokens.size() < 2) {
        throw std::invalid_argument(
                "TraceFileLoader::loadFromTraceFile(): batch workload trace file name must end with '.swf' or 'json'");
      }
      if (tokens[tokens.size() - 1] == "swf") {
        return loadFromTraceFileSWF(filename, load_time_compensation);
      } else if (tokens[tokens.size() - 1] == "json") {
        return loadFromTraceFileJSON(filename, load_time_compensation);
      } else {
        throw std::invalid_argument(
                "TraceFileLoader::loadFromTraceFile(): batch workload trace file name must end with '.swf' or 'json'");
      }
    }

    /**
    * @brief Load the workflow SWF trace file
    *
    * @param filename: the path to the trace file in SWF format
    * @param load_time_compensation: an offset to add to submit times in the trace file
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int>>
    TraceFileLoader::loadFromTraceFileSWF(std::string filename, double load_time_compensation) {

      std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> trace_file_jobs = {};

      std::ifstream infile(filename);
      if (not infile.is_open()) {
        throw std::invalid_argument("TraceFileLoader::loadFromTraceFileSWF(): Cannot open batch workload trace file " + filename);
      }

      try {
        std::string line;
        while (std::getline(infile, line)) {
          if (line[0] != ';') {
            std::istringstream iss(line);
            std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                            std::istream_iterator<std::string>{}};

            if (tokens.size() < 10) {
              throw std::invalid_argument(
                      "TraceFileLoader::loadFromTraceFileSWF(): Seeing less than 10 fields per line in batch workload trace file '" +
                      filename +
                      "'");
            }

            std::string id;
            double time = -1, requested_time = -1, requested_ram = -1;
            int itemnum = 0;
            double sub_time = -1;
            int requested_num_nodes = -1;
            int num_nodes = -1;
            for (auto item:tokens) {
              switch (itemnum) {
                case 0: // Job ID
                  id = item;
                  break;
                case 1: // Submit time
                  if (sscanf(item.c_str(), "%lf", &sub_time) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid submission time '" + item +
                            "' in batch workload trace file");
                  }
                  sub_time += load_time_compensation;
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
                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid number of processors '" + item +
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
                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested number of processors '" + item +
                            "' in batch workload trace file");
                  }
                  break;
                case 8: // Requested time
                  //assuming flops and runtime are the same (in seconds)
                  if (sscanf(item.c_str(), "%lf", &requested_time) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested time '" + item +
                            "' in batch workload trace file");
                  }
                  break;
                case 9: // Requested memory
                  // In KiB
                  if (sscanf(item.c_str(), "%lf", &requested_ram) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFileSWF(): Invalid requested memory '" + item +
                            "' in batch workload trace file");
                  }
                  requested_ram *= 1024.0;
                  break;
                case 10: // Status
                  break;
                case 11: // User ID
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
                  throw std::runtime_error(
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
                      "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative flops and negative requested flops in batch workload trace file");
            }
            if (requested_ram < 0) {
              requested_ram = 0;
            }
            if (sub_time < 0) {
              throw std::invalid_argument(
                      "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative submission time in batch workload trace file");
            }
            if (requested_num_nodes < 0) {
              requested_num_nodes = num_nodes;
            }
            if (requested_num_nodes < 0) {
              throw std::invalid_argument(
                      "TraceFileLoader::loadFromTraceFileSWF(): invalid job with negative (requested) number of node in batch workload trace file");
            }

            // Add the job to the list
            std::tuple<std::string, double, double, double, double, unsigned int> job =
                    std::tuple<std::string, double, double, double, double, unsigned int>(
                            id, sub_time, time, requested_time, requested_ram, (unsigned int) requested_num_nodes);
            trace_file_jobs.push_back(job);
          }
        }
      } catch (std::exception &e) {
        throw std::invalid_argument("Errors while reading batch workload trace file '" + filename + "': " + e.what());
      }
      return trace_file_jobs;
    }

    /**
    * @brief Load the workflow JSON trace file
    *
    * @param filename: the path to the trace file in JSON format
    * @param load_time_compensation: an offset to add to submit times in the trace file
    *
    * @return  a vector of tuples, where each tuple is a job description with the following fields:
    *              - job id
    *              - submission time
    *              - actual time
    *              - requested time,
    *              - requested ram
    *              - requested number of nodes
    *
    * @throw std::invalid_argument
    */
    std::vector<std::tuple<std::string, double, double, double, double, unsigned int>>
    TraceFileLoader::loadFromTraceFileJSON(std::string filename, double load_time_compensation) {

      std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> trace_file_jobs = {};

      std::ifstream file;
      nlohmann::json j;


      //handle the exceptions of opening the json file
      file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      try {
        file.open(filename);
      } catch (const std::ifstream::failure &e) {
        throw std::invalid_argument("TraceFileLoader::loadFromTraceFileJSON(): Cannot open JSON batch workload trace file " + filename);
      }


      try {
        file >> j;
        file.close();
      } catch (const std::exception &e) {
        throw std::invalid_argument(std::string("TraceFileLoader::loadFromTraceFileJSON(): JSON parse error: ") + e.what());
      }

      nlohmann::json jobs;
      try {
        jobs = j.at("jobs");
      } catch (std::exception &e) {
        throw std::invalid_argument("TraceFileLoader::loadFromTraceFileJSON(): Could not find 'jobs' in JSON batch workload trace file");
      }

      for (nlohmann::json::iterator it = jobs.begin(); it != jobs.end(); ++it) {
        nlohmann::json json_job = it.value();
        if (not json_job.is_object()) {
          // Broken JSON
          throw std::invalid_argument("TraceFileLoader::loadFromTraceFileJSON(): broken job specification in JSON batch workload trace file");
        }

        unsigned id;
        unsigned long res;
        double subtime;
        double walltime;

        try {
          id = json_job.at("id");
          res = json_job.at("res");
          subtime = json_job.at("subtime");
          walltime = json_job.at("walltime");
        } catch (std::exception &e) {
          throw std::invalid_argument("TraceFileLoader::loadFromTraceFileJSON(): invalid job specification in JSON batch workload trace file");
        }

        // Fix/check values
        if ((res <= 0) or (subtime < 0) or (walltime < 0)) {
          throw std::invalid_argument("TraceFileLoader::loadFromTraceFileJSON(): invalid job specification in JSON batch workload trace file");
        }
        res += load_time_compensation;

        // TODO: What the deal with requested times????
        // For now, just use the walltime...
        double requested_time = walltime;

//      // Add the job to the list
        std::tuple<std::string, double, double, double, double, unsigned int> job =
                std::tuple<std::string, double, double, double, double, unsigned int>(
                        std::to_string(id), subtime, walltime, requested_time, 0.0, (unsigned int) res);
        trace_file_jobs.push_back(job);
      }

      return trace_file_jobs;
    }
}

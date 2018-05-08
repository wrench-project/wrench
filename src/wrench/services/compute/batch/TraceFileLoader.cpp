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
#include "wrench/util/TraceFileLoader.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(trace_file_loader, "Log category for Trace File Loader");

namespace wrench {

    /**
    * @brief Load the workflow trace file
    *
    * @param filename: the path to the trace file
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

      std::vector<std::tuple<std::string, double, double, double, double, unsigned int>> trace_file_jobs = {};

      std::ifstream infile(filename);
      if (not infile.is_open()) {
        throw std::invalid_argument("TraceFileLoader::loadFromTraceFile(): Cannot open trace file " + filename);
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
                      "TraceFileLoader::loadFromTraceFile(): Seeing less than 10 fields per line in trace file '" +
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
                            "TraceFileLoader::loadFromTraceFile(): Invalid submission time in trace file '" + item +
                            "'");
                  }
                  sub_time += load_time_compensation;
                  break;
                case 2: // Wait time
                  break;
                case 3: // Run time
                  //assuming flops and runtime are the same (in seconds)
                  if (sscanf(item.c_str(), "%lf", &time) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFile(): Invalid run time in trace file '" + item +
                            "'");
                  }
                  break;
                case 4: // Number of Allocated Processors
                  if (sscanf(item.c_str(), "%d", &num_nodes) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFile(): Invalid number of processors '" + item +
                            "'");
                  }
                  break;
                case 5:// Average CPU time Used
                  break;
                case 6: // Used Memory
                  break;
                case 7: // Requested Number of Processors
                  if (sscanf(item.c_str(), "%d", &requested_num_nodes) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFile(): Invalid requested number of processors '" + item +
                            "'");
                  }
                  break;
                case 8: // Requested time
                  //assuming flops and runtime are the same (in seconds)
                  if (sscanf(item.c_str(), "%lf", &requested_time) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFile(): Invalid requested time in trace file '" + item +
                            "'");
                  }
                  break;
                case 9: // Requested memory
                  // In KiB
                  if (sscanf(item.c_str(), "%lf", &requested_ram) != 1) {
                    throw std::invalid_argument(
                            "TraceFileLoader::loadFromTraceFile(): Invalid requested memory in trace file '" + item +
                            "'");
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
                          "TraceFileLoader::loadFromTraceFile(): Unknown trace file column, may be there are more than 18 columns in trace file"
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
                      "TraceFileLoader::loadFromTraceFile(): invalid job with negative flops and negative requested flops");
            }
            if (requested_ram < 0) {
              requested_ram = 0;
            }
            if (sub_time < 0) {
              throw std::invalid_argument(
                      "TraceFileLoader::loadFromTraceFile(): invalid job with negative submission time");
            }
            if (requested_num_nodes < 0) {
              requested_num_nodes = num_nodes;
            }
            if (requested_num_nodes < 0) {
              throw std::invalid_argument(
                      "TraceFileLoader::loadFromTraceFile(): invalid job with negative (requested) number of node");
            }

            // Add the job to the list
            std::tuple<std::string, double, double, double, double, unsigned int> job =
                    std::tuple<std::string, double, double, double, double, unsigned int>(
                            id, sub_time, time, requested_time, requested_ram, (unsigned int) requested_num_nodes);
            trace_file_jobs.push_back(job);
          }
        }
      } catch (std::exception &e) {
        throw std::invalid_argument("Errors while reading workload trace file '" + filename + "': " + e.what());
      }
      return trace_file_jobs;
    }
}

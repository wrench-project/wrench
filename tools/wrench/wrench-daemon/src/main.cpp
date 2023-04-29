/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "httplib.h"

#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <WRENCHDaemon.h>

namespace po = boost::program_options;

/**
 * @brief main method of wrench-daemon
 *
 * @param argc number of command-line arguments
 * @param argv command-line arguments
 *
 * @return exit code
 */
int main(int argc, char **argv) {
    // Generic lambda to check if a numeric argument is in some range
    auto in = [](const auto &min, const auto &max, char const *const opt_name) {
        return [opt_name, min, max](const auto &v) {
            if (v < min || v > max) {
                throw po::validation_error(po::validation_error::invalid_option_value,
                                           opt_name, std::to_string(v));
            }
        };
    };

    // Define command-line argument options
    po::options_description desc("Allowed options");
    desc.add_options()("help", "Show this help message")("simulation-logging", po::bool_switch()->default_value(false),
                                                         "Show full simulation log during execution")("daemon-logging", po::bool_switch()->default_value(false),
                                                                                                      "Show full daemon log during execution")("port", po::value<int>()->default_value(8101)->notifier(in(1024, 49151, "port")),
                                                                                                                                               "port number, between 1024 and 4951, on which this daemon will listen")("sleep-us", po::value<int>()->default_value(200)->notifier(in(0, 1000000, "sleep-us")),
                                                                                                                                                                                                                       "number of micro-seconds, between 0 and 1000000, that the simulation thread sleeps at each iteration of its main loop (smaller means faster simulation, larger means less CPU load)");

    // Parse command-line arguments
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        // Print help message and exit if needed
        if (vm.count("help")) {
            cout << desc << "\n";
            exit(0);
        }
        // Throw whatever exception in case argument values are erroneous
        po::notify(vm);
    } catch (std::exception &e) {
        cerr << "Error: " << e.what() << "\n";
        exit(1);
    }

    // Create and run the WRENCH daemon
    WRENCHDaemon daemon(vm["simulation-logging"].as<bool>(),
                        vm["daemon-logging"].as<bool>(),
                        vm["port"].as<int>(),
                        vm["sleep-us"].as<int>());

    daemon.run();// Should never return

    exit(0);
}

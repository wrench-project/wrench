/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "SimulationLauncher.h"

#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#include <WRENCHDaemon.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <SimulationDaemon.h>

using json = nlohmann::json;

// Range of ports that simulation daemons can listen on
#define PORT_MIN 10000
#define PORT_MAX 20000

/**
 * @brief Constructor
 * @param simulation_logging true if simulation logging should be printed
 * @param daemon_logging true if daemon logging should be printed
 * @param port_number port number on which to listen
 * @param sleep_us number of micro-seconds or real time that the simulation daemon's simulation controller
 *        thread should sleep at each iteration
 */
WRENCHDaemon::WRENCHDaemon(bool simulation_logging,
                           bool daemon_logging,
                           int port_number,
                           int sleep_us) :
        simulation_logging(simulation_logging),
        daemon_logging(daemon_logging),
        port_number(port_number),
        sleep_us(sleep_us) {}

/**
 * @brief Helper method to check whether a port is available for binding/listening
 * @param port the port number
 * @return true if the port is taken, false if it is available
 */
bool WRENCHDaemon::isPortTaken(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    auto ret_value = ::bind(sock, (struct sockaddr *) &address, sizeof(address));

    if (ret_value == 0) {
        close(sock);
        return false;
    } else if (errno == EADDRINUSE) {
        close(sock);
        return true;
    } else {
        throw std::runtime_error("isPortTaken(): port should be either taken or not taken");
    }
}

/**
 * @brief Helper function to set up a "success" HTTP answer
 * @param res the response object to update
 * @param port_number the port_number on which simulation client will need to connect
 */
void setSuccessAnswer(Response &res, int port_number) {
    json answer;
    answer["wrench_api_request_success"] = true;
    answer["port_number"] = port_number;
    res.set_header("access-control-allow-origin", "*");
    res.set_content(answer.dump(), "application/json");
}

/**
 * @brief Helper function to set up a "failure" HTTP answer
 * @param res res the response object to update
 * @param failure_cause a human-readable error message
 */
void setFailureAnswer(Response &res, const std::string &failure_cause) {
    json answer;
    answer["wrench_api_request_success"] = false;
    answer["failure_cause"] = failure_cause;
    res.set_header("access-control-allow-origin", "*");
    res.set_content(answer.dump(), "application/json");
}

/**
 * @brief Helper function to write a string to a shared-memory segment
 * @param shm_segment_id shared-memory segment ID
 * @param content string to write
 */
void writeStringToSharedMemorySegment(int shm_segment_id, const std::string &content) {
    auto shm_segment = (char *) shmat(shm_segment_id, nullptr, 0);
    if ((long) shm_segment == -1) {
        perror("WARNING: shmat(): ");
        return;
    }
    strcpy(shm_segment, content.c_str());
    if (shmdt(shm_segment) == -1) {
        perror("WARNING: shmdt()");
        return;
    }
}

/**
 * @brief Helper function to read a string from a shared-memory segment
 * @param shm_segment_id shared-memory segment ID
 * @return string read
 */
std::string readStringFromSharedMemorySegment(int shm_segment_id) {
    auto shm_segment = (char *) shmat(shm_segment_id, nullptr, 0);
    if ((long) shm_segment == -1) {
        perror("WARNING: shmat(): ");
        return "n/a";
    }
    std::string to_return = std::string(shm_segment);
    if (shmdt(shm_segment) == -1) {
        perror("WARNING: shmdt()");
    }
    return to_return;
}

/**
 * @brief REST API Handler
 * @param req HTTP request
 * @param res HTTP response
 *
 * BEGIN_REST_API_DOCUMENTATION
 * {
 *   "REST_func": "startSimulation",
 *   "documentation":
 *     {
 *       "purpose": "Start a new simulation",
 *       "json_input": {
 *         "platform_xml": ["string", "XML description of the simulated platform (using the SimGrid DTD)"],
 *         "controller_hostname": ["string", "Name of the host in the simulated platform that runs the simulation controller"]
 *       },
 *       "json_output": {
 *         "port_number": ["int", "port number to which all subsequent HTTP Post requests should be sent"]
 *       }
 *     }
 * }
 * END_REST_API_DOCUMENTATION
 */
void WRENCHDaemon::startSimulation(const Request &req, Response &res) {
    // Print some logging
    unsigned long max_line_length = 120;
    if (daemon_logging) {
        std::cerr << req.path << " " << req.body.substr(0, max_line_length)
                  << (req.body.length() > max_line_length ? "..." : "") << std::endl;
    }

    // Parse the HTTP request's data
    json body;
    try {
        body = json::parse(req.body);
    } catch (std::exception &e) {
        setFailureAnswer(res, "Internal error: malformed json in request");
        return;
    }

    // Find an available port number on which the simulation daemon will be able to run
    int simulation_port_number;
    while (isPortTaken(simulation_port_number = PORT_MIN + rand() % (PORT_MAX - PORT_MIN)));

    // Create a shared memory segment, to which an error message will be written by
    // the child process (the simulation daemon) in case it fails on startup
    // due to a simulation creation failure
    auto shm_segment_id = shmget(IPC_PRIVATE, 4096, IPC_CREAT | SHM_R | SHM_W);
    if (shm_segment_id == -1) {
        perror("shmget()");
        setFailureAnswer(res, "Internal wrench-daemon error: shmget(): " + std::string(strerror(errno)));
        return;
    }

    // Write a nondescript error message in the shared-memory segment in case
    // writing to the shared-memory segment later fails (which it shouldn't)
    writeStringToSharedMemorySegment(shm_segment_id, "Internal wrench-daemon error: ");

    // Create a child process
    auto child_pid = fork();
    if (child_pid == -1) {
        perror("fork()");
        setFailureAnswer(res, "Internal wrench-daemon error: fork(): " + std::string(strerror(errno)));
        return;
    }

    if (!child_pid) { // The child process
        // Stop the server that was listening on the main WRENCH daemon port
        server.stop();

        // Create a pipe for communication with my child (aka the grand-child)
        int fd[2];
        if (pipe(fd) == -1) {
            perror("pipe()");
            writeStringToSharedMemorySegment(shm_segment_id,
                                             "Internal wrench-daemon error: pipe(): " + std::string(strerror(errno)));
            exit(1);
        }

        // The child process creates a grand child, that will be adopted by
        // pid 1 (the well-known "if I create a child that creates a grand-child
        // and I kill the child, then my grand-child will never become a zombie" trick). 
        // This trick is a life-saver here, since setting up a SIGCHLD handler
        // to reap children would get in the way of what we need to do in the code hereafter.
        auto grand_child_pid = fork();
        if (grand_child_pid == -1) {
            perror("fork()");
            writeStringToSharedMemorySegment(shm_segment_id,
                                             "Internal wrench-daemon error: fork(): " + std::string(strerror(errno)));
            exit(1);
        }

        if (!grand_child_pid) { // the grand-child

            // close read-end of the pipe
            close(fd[0]);

            // Create the simulation launcher
            auto simulation_launcher = new SimulationLauncher();

            // mutex/condvar for synchronization with the simulation thread I am about to create
            std::mutex guard;
            std::condition_variable signal;

            // Create AND launch the simulation in a separate thread (it is tempting to
            // do the creation here and the launch in a thread, but it seems that SimGrid
            // does not like that - likely due to the maestro business)
            auto simulation_thread = std::thread([simulation_launcher, this, body, &guard, &signal]() {
                // Create simulation
                simulation_launcher->createSimulation(this->simulation_logging,
                                                      body["platform_xml"],
                                                      body["controller_hostname"],
                                                      this->sleep_us);
                // Signal the parent thread that simulation creation has been done, successfully or not
                {
                    std::unique_lock <std::mutex> lock(guard);
                    signal.notify_one();
                }
                // If no failure, then proceed with the launch!
                if (not simulation_launcher->launchError()) {
                    simulation_launcher->launchSimulation();
                }
            });

            // Waiting for the simulation thread to have created the simulation, successfully or not
            {
                std::unique_lock <std::mutex> lock(guard);
                signal.wait(lock);
            }

            // If there was a simulation launch error, then put the error message in the
            // shared memory segment, communicate the failure to the parent process
            // via a pipe,  and exit
            if (simulation_launcher->launchError()) {
                simulation_thread.join(); // THIS IS NECESSARY, otherwise the exit silently segfaults!
                // Put the error message in shared memory segment
                writeStringToSharedMemorySegment(shm_segment_id, simulation_launcher->launchErrorMessage());

                // Write success status to the parent
                bool success = false;
                if (write(fd[1], &success, sizeof(bool)) == -1) {
                    perror("WARNING: write()"); // just a warning, since we're already in error mode anyway
                }
                // Close the write-end of the pipe
                close(fd[1]);
                // Terminate with a non-zero error code, just for kicks (nobody's calling waitpid)
                exit(1);

            } else {
                // Write to the pipe that everything's ok and close it
                bool success = true;
                if (write(fd[1], &success, sizeof(bool)) == -1) {
                    perror("write()");
                    writeStringToSharedMemorySegment(shm_segment_id, "Internal wrench-daemon error: write(): " +
                                                                     std::string(strerror(errno)));
                    exit(1);
                }

                // Close the write-end of the pipe
                close(fd[1]);

                // Create a simulation daemon
                auto simulation_daemon = new SimulationDaemon(
                        daemon_logging, simulation_port_number,
                        simulation_launcher->getController(), simulation_thread);

                // Start the HTTP server for this particular simulation
                simulation_daemon->run(); // never returns
                exit(0); // never executed
            }

        } else {
            // close the write-end of the pipe
            close(fd[1]);

            // Wait to hear from the child via the pipe
            bool success;
            if (read(fd[0], &success, sizeof(bool)) == -1) {
                // If broken pipe, when we know it's a failure
                success = false;
            }

            // If success exit(0) otherwise exit(1)
            exit(success ? 0 : 1);
        }

    } else { // The parent process
        // Wait for the child to finish and get its exit code
        int stat_loc;
        if (waitpid(child_pid, &stat_loc, 0) == -1) {
            perror("waitpid()");
            setFailureAnswer(res, "Internal wrench-daemon error: waitpid(): " + std::string(strerror(errno)));
            return;
        }

        // Create json answer that will inform the client of success or failure, based on
        // child's exit code (which was relayed to this process from the grand-child)
        if (WEXITSTATUS(stat_loc) == 0) {
            setSuccessAnswer(res, simulation_port_number);
        } else {
            // Grab the error message from the shared memory segment and set up the failure answer
            setFailureAnswer(res, readStringFromSharedMemorySegment(shm_segment_id));
        }

        // Destroy the shared memory segment (important, since there is a limited
        // number of them we can create, and besides we should clean-up after ourselves)
        if (shmctl(shm_segment_id, IPC_RMID, nullptr) == -1) {
            perror("WARNING: shmctl()");
        }

        // At this point, the answer has been set
        return;
    }
}

/**
 * @brief A generic error handler that simply prints some information
 * @param req the HTTP request
 * @param res the HTTP response
 */
void WRENCHDaemon::error_handling(const Request &req, Response &res) {
    std::cerr << "[" << res.status << "]: " << req.path << " " << req.body << "\n";
}

/**
 * @brief The WRENCH daemon's "main" method
 */
void WRENCHDaemon::run() {
    // Only set up POST request handler for "/api/startSimulation" since
    // all other API paths will be handled by a simulation daemon instead
    server.Post("/api/startSimulation", [this](const Request &req, Response &res) { this->startSimulation(req, res); });

    // Set some generic error handler
    server.set_error_handler([](const Request &req, Response &res) { WRENCHDaemon::error_handling(req, res); });

    // Start the web server
    if (daemon_logging) {
        std::cerr << "WRENCH daemon listening on port " << port_number << "...\n";
    }
    while (true) {
        // This is in a while loop because, on Linux, the main process seems to return
        // from the listen() call below, not sure why... perhaps this while loop
        // could be removed, but it likely doesn't hurt
        server.listen("0.0.0.0", port_number);
    }
}

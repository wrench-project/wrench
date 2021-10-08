Interacting with a bare-metal compute service  {#guide-102-baremetal}
=======

A `wrench::StandardJob` can be submitted to a bare-metal compute service
via a job manager. For instance:

~~~~~~~~~~~~~{.cpp}
// Get the first bare-metal compute service (assuming there's at least one)
auto bare_metal_service = *(this->getAvailableComputeServices<wrench::BareMetalComputeService>().begin());

// Create a job manager
auto job_manager = this->createJobManager();

// Create a simple standard job with 4 workflow tasks
auto job = job_manager->createStandardJob(
                 {this->getWorklow()->getTaskByID("task1"),
                  this->getWorklow()->getTaskByID("task2"),
                  this->getWorklow()->getTaskByID("task3"),
                  this->getWorklow()->getTaskByID("task4")});

// Submit the job to the bare-metal service
job_manager->submitJob(job, baremetal_cs);

//  Wait for and process the next event (should be a standard job completion or failure)
this->waitForAndProcessNextEvent();
~~~~~~~~~~~~~

In the above, the bare-metal service will make all decisions for deciding
how to allocate compute resources (i.e., cores) to tasks. In fact, several
properties (see class `wrench::BareMetalComputeServiceProperty`) can be set
to change the algorithms used by the service to determine resource
allocations.

In some cases, the WMS may want to influence or enforce resource
allocations for the tasks in the jobs. For this purpose,  the
`wrench::JobManager::submitJob()` method takes an optional
**service-specific argument**. This  argument is a `std::map<std::string,
std::string>` of key-value pairs. The key is a task ID, and the value is
the service-specific argument for that task.

For each task, an optional argument can be provided as a string formatted
as "hostname:num_cores", "hostname", or "num_cores", where "hostname" is the name
of one of the service's compute hosts and "num_cores" is an integer (e.g., "host1:10",
"host1", "10"):

  - If no value is provided for a task, or if the value is the empty string, then the bare-metal
    service will choose the host on which the task should be executed (typically the host with
    the lowest current load), and will execute the task with as many cores as possible on that host. 
  
  - If a "hostname" value is provided for a task, then the bare-metal service will execute the
    task on that host, and will execute the task with as many cores as possible on that host.

  - If a "num_cores" value is provided for a task, then the bare-metal
    service will choose the host on which the task should be executed (typically the host with
    the lowest current load), and will execute the task with the specified number of cores. 

  - If a "hostname:num_cores" value is provided for a task, then the bare-metal service
   will execute the task on that host with the specified number of cores.


In the above  example, for instance, the job  submission could be done  as: 
~~~~~~~~~~~~~{.cpp}
[...]

// Create a service-specific argument
std::map<std::string, std::string> service_specific_args;

// task1 will run on host Node1 with as many cores as possible
service_specific_args["task1"] = "Node1";

// task2 will run on host Node2 with 16 cores
service_specific_args["task2"] = "Node2:16";

// task3 will run on any host with as many cores as possible
service_specific_args["task3"] = "";  // could be omitted altogether

// task4 will run on some host with 4 cores
service_specific_args["task4"] = "4";

// Submit the job
job_manager->submitJob(job, baremetal_cs, service_specific_args);

[...]
~~~~~~~~~~~~~

If the service-specific arguments are invalid (e.g., invalid hostname, unknown task, 
number of cores too large), the `wrench::JobManager::submitJob()` method 
throws a `wrench::WorkflowExecutionException`.

See the WMS implementation in `examples/basic-examples/bare-metal-bag-of-tasks/TwoTasksAtATimeWMS.cpp` for a more complete example.


---

Interacting with a batch compute service         {#guide-102-batch}
============

A job, either `wrench::StandardJob` or `wrench::PilotJob`, 
can be submitted to a `wrench::BatchComputeService` by a  call
to the `wrench::JobManager::submitJob()` method. However, it is
**required** to pass to it a service-specific argument. This argument
is a `std::map<std::string, std::string>` of
key-value pairs, and must have the following three elements:

  - key: `-t`; value: a requested runtime in minutes that, if exceeded, causes forceful job termination (e.g., "60");
  - key: `-N`; value: a requested number of compute nodes (e.g., "2"); and
  - key: `-c`; value: a requested number of cores per compute nodes (e.g., "4").

You may note that the above corresponds to the arguments that must be
provided to the [Slurm](https://slurm.schedmd.com/) batch scheduler.

Here is an example job submission to the batch service:

~~~~~~~~~~~~~{.cpp}
// Get the first batch compute service (assuming there's at least one)
auto batch_service = *(this->getAvailableComputeServices<wrench::BatchComputeService>().begin());

// Create a job manager
auto job_manager = this->createJobManager();

// Create a job
auto job = job_manager->createStandardJob(tasks, {});

// Create service-specific arguments
std::map<std::string, std::string> service_specific_args;

//   The job will run no longer than 1 hour
service_specific_args["-t"] = "60";

//   The job will run on 2 compute nodes
service_specific_args["-N"] = "2";

//   The job will use 4 cores on each compute nodes
service_specific_args["-c"] = "4";

// Submit the job
job_manager->submitJob(job, batch_cs, service_specific_args);

//  Wait for and process the next event
this->waitForAndProcessNextEvent();
~~~~~~~~~~~~~

If the service-specific arguments are invalid (e.g., number of nodes too
large), `wrench::JobManager::submitJob()` method throws a
`wrench::WorkflowExecutionException`.

See the WMS implementation in `examples/basic-examples/batch-bag-of-tasks/TwoTasksAtATimeBatchWMS.cpp` for a more complete example.


A batch compute service also supports pilot jobs. Once started, a pilot job exposes a temporary  (only running until its containing pilot job expires) bare-metal compute service. Here is a simple code excerpt:

~~~~~~~~~~~~~{.cpp}
// create a pilot job
auto pilot_job = job_manager->createPilotJob();

// submit it to the batch compute service, asking for 2 10-core nodes for 20 minutes
std::map<std::string, std::string> service_specific_arguments = 
            {{"-N","2"},{"-c","10"},{"-t","20"}};
job_manager->submitJob(pilot_job, batch_service, service_specific_arguments);

// Waiting for the next event (which will be a pilot job start event)
this->waitForAndProcessNextEvent();

// Get a reference to the bare-metal compute service running on the pilot job
auto cs = pilot_job->getComputeService();

// Start using the bare-metal compute service
[...]

~~~~~~~~~~~~~

While the pilot job is running,  [standard jobs can be submitted to its bare-metal service](@ref guide-102-baremetal).


See the WMS implementation in `examples/basic-examples/basic-examples/batch-pilot-job/PilotJobWMS.cpp` for a more complete example.


Interacting with a HTCondor compute service {#guide-102-htcondor}
============


A `wrench::HTCondorComputeService` instance is essential a front-end to several 
"child" compute services. As such one can submit jobs to it, just like one would do to
other compute services, and it may "decide" to which service these jobs
will be delegated. In fact, a WMS can even add new child compute services  to be used
by HTCondor dynamically. Which child service is used is dictated/influenced
by service-specific arguments passed to the `wrench::JobManager::submitJob()` method. 


Rather than going into a long-winded explanation, the examples code-fragment below
show-case the creation of a `wrench::HTCondorComputeService` instance
and its use by a WMS.  Let's start with the creation (in main). Note that
arguments to service constructors are omitted for brevity (see the WMS
implementation in `examples/condor-grid-example/CondorWMS.cp` for a
complete and working example).


~~~~~~~~~~~~~{.cpp}
// Create a BareMetalComputeService instance
auto baremetal_cs = simulation->add(new wrench::BareMetalComputeService(...));

// Create two BatchComputeService instances
auto batch1_cs = simulation->add(new wrench::BatchComputeService(...));
auto batch2_cs = simulation->add(new wrench::BatchComputeService(...));

// Create a HTCondorComputeService instance with the above 
// three services as "child" services
auto htcondor_cs = simulation->add(
     new wrench::HTCondorComputeService("some_host", {baremetal_cs, batch1_cs, batch2_cs}, "/scratch");

// Create a CloudComputeService instance
auto cloud_cs = simulation->add(new wrench::CloudComputeService(...));
~~~~~~~~~~~~~

Let's now say that a WMS was created that has access to all 5 above services, but will choose to submit
all jobs via HTCondor. The first thing to do, so as to make the use of the cloud service possible,
is to create a few VM instances and add them as child services to the HTCondor service:

~~~~~~~~~~~~~{.cpp}
// Create and start to VMs on the cloud service
auto vm1 = cloud_cs->createVM(...);
auto vm2 = cloud_cs->createVM(...);
auto vm1_cs = cloud_cs->startVM(vm1); 
auto vm2_cs = cloud_cs->startVM(vm1);

// Add the two VM's bare-metal compute services to HTCondor
htcondor_cs->addComputeService(vm1_cs);
htcondor_cs->addComputeService(vm2_cs);
~~~~~~~~~~~~~

So, at this point, HTCondor has access to 3 bare-metal compute services (2 of which are running inside VMs),
and 2 batch compute services.

Let's consider that the WMS will submit `wrench::StandardJob` instances to HTCondor. These jobs can be
of two kinds or, in HTCondor parlance, belong to one of two universes: **grid** jobs and **non-grid** jobs. 
By default a job is considered to be in the non-grid universe. But if the service-specific arguments
passed to `wrench::JobManager::submitJob()` include a "universe":"grid" key:value pair, then the submitted job
is in the grid universe.  HTCondor handles both kinds of jobs differently:

  - Non-grid universe jobs are queued  and dispatched by HTCondor whenever
    possible to idle resources managed by one of the child bare-metal
    services.  HTCondor chooses the service to use.

  - Grid universe jobs are dispatched by HTCondor immediately to 
    a specific child batch compute service. As a result, these jobs
    must be submitted with service-specific arguments that provide values
    for "-N", "-c", and "-t" keys (like for any job submitted to a batch
    compute service), as well as a "-service" key that specifies the name
    of the batch service that should run the job (this argument is optional
    if there is a single child batch compute service). 

In the example below, we show both kinds of job submissions:

~~~~~~~~~~~~~{.cpp}
// Create a non-grid universe standard job and submit it to HTCondor,
// which will run it on one of its 3 child bare-metal compute services
auto ng_job = job_manager->createStandardJob(...);
job_manager->submitJob(ng_job, htcondor_cs, {}); // no service-specific arguments

// Create a grid universe standard job and submit it to HTCondor,
// which will run it on a specific child batch compute service. 
auto n_job = job_manager->createStandardJob(...);

std::map<std::string, std::string> service_specific_args;
service_specific_args["-N"] = "2";          // 2 compute nodes
service_specific_args["-c"] = "4";          // 4 cores per compute nodes
service_specific_args["-t"] = "60";         // runs for one hour
service_specific_args["universe"] = "grid"; // Grid universe
service_specific_args["-service"] = batch1_cs->getName(); // Run on the first batch compute service

job_manager->submitJob(n_job, htcondor_cs, service_specific_args); // no service-specific arguments
~~~~~~~~~~~~~

The above covers the essentials. See the code in the `examples/condor-grid-example/` directory
for working/usable code. 



# Anatomy of the HTCondor Service #        {#guide-htcondor-anatomy}

The in-simulation implementation of HTCondor in WRENCH is simplified in
terms of its functionality and design when compared to the actual
implementation of HTCondor. The `wrench::HTCondorComputeService` spawns two
additional services during execution,
`wrench::HTCondorCentralManagerService` and
`wrench::HTCondorNegotiatorService`, both of which loosely correspond to
actual HTCondor daemons (`collector`, `negotiator`, `schedd`).


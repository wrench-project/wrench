.. _guide-102-htcondor:

Interacting with a HTCondor compute service
===========================================

A :cpp:class:`wrench::HTCondorComputeService` instance is essentially a front-end
to several “child” compute services. As such, one can submit jobs to it,
just like one would do to any compute service, but it then “decides” to
which service these jobs will be delegated. In fact, an execution
controller can even add new child compute services to be used by
HTCondor dynamically. Which child service is used is dictated/influenced
by service-specific arguments passed or not passed to the
:cpp:class:`wrench::JobManager::submitJob()` method.

The examples code fragments below showcase the creation of a
:cpp:class:`wrench::HTCondorComputeService` instance and its use by an execution
controller. Let’s start with the creation (in main). Note that arguments
to service constructors are omitted for brevity (see the execution
controller implementation in
``examples/workflow_api/condor-grid-example/CondorWMS.cp`` for a complete and working
example).

.. code:: cpp

   // One BareMetalComputeService instance
   std::shared_ptr<wrench::BareMetalComputeService> some_baremetal_cs;

   // Two BatchComputeService instances
   std::shared_ptr<wrench::BatchComputeService> some_batch1_cs;
   std::shared_ptr<wrench::BatchComputeService> some_batch2_cs;

   // Create a HTCondorComputeService instance with the above 
   // three services as "child" services
   auto htcondor_cs = simulation->add(
        new wrench::HTCondorComputeService("some_host", 
                                           {some_baremetal_cs, some_batch1_cs, some_batch2_cs}, 
                                           "/scratch");

   // One CloudComputeService instance
   std::shared_ptr<wrench::CloudComputeService> somecloud_cs;

Let’s now say that an execution controller was created that has access
to all 5 above services, but will choose to submit all jobs via
HTCondor. The first thing to do, so as to make the use of the cloud
service possible, is to create a few VM instances and add them as child
services to the HTCondor service:

.. code:: cpp

   // Create and start to VMs on the cloud service
   auto vm1 = some_cloud_cs->createVM(...);
   auto vm2 = some_cloud_cs->createVM(...);
   auto vm1_cs = some_cloud_cs->startVM(vm1); 
   auto vm2_cs = some_cloud_cs->startVM(vm2);

   // Add the two VM's bare-metal compute services to HTCondor
   htcondor_cs->addComputeService(vm1_cs);
   htcondor_cs->addComputeService(vm2_cs);

So, at this point, HTCondor has access to 3 bare-metal compute services
(2 of which are running inside VMs), and 2 batch compute services.

Let’s consider an execution controller that will submit
:cpp:class:`wrench::StandardJob` instances to HTCondor. These jobs can be of two
kinds or, in HTCondor parlance, belong to one of two universes: **grid**
jobs and **non-grid** jobs. By default a job is considered to be in the
non-grid universe. But if the service-specific arguments passed to
:cpp:class:`wrench::JobManager::submitJob()` include a “universe”:“grid”
key:value pair, then the submitted job is in the grid universe. HTCondor
handles both kinds of jobs differently:

-  Non-grid universe jobs are queued and dispatched by HTCondor whenever
   possible to idle resources managed by one of the child bare-metal
   services. HTCondor chooses the service to use based on availability
   of resources.

-  Grid universe jobs are dispatched by HTCondor immediately to a
   specific child batch compute service. As a result, these jobs must be
   submitted with service-specific arguments that provide values for
   “-N”, “-c”, and “-t” keys (like for any job submitted to a batch
   compute service), as well as a “-service” key that specifies the name
   of the batch service that should run the job (this argument is
   optional if there is a single child batch compute service).

In the example below, we show both kinds of job submissions:

.. code:: cpp

   // Create a standard job and submit it to HTCondor as a non-grid job,
   // which will thus run it on one of its 3 child bare-metal compute services
   auto ng_job = job_manager->createStandardJob(...);
   job_manager->submitJob(ng_job, htcondor_cs, {}); // no service-specific arguments

   // Create a standard job and submit it to HTCondor as a grid job,
   // which will run it on the specified child batch compute service. 
   auto g_job = job_manager->createStandardJob(...);

   std::map<std::string, std::string> service_specific_args;
   service_specific_args["-N"] = "2"; // 2 compute nodes
   service_specific_args["-c"] = "4"; // 4 cores per compute nodes
   service_specific_args["-t"] = "60"; // runs for one hour
   service_specific_args["universe"] = "grid"; // Grid universe
   // Set it to run on the first batch compute service
   service_specific_args["-service"] = batch1_cs->getName(); 

   job_manager->submitJob(g_job, htcondor_cs, service_specific_args);

The above covers the essentials. See the API documnetation for more
options, and the code in the ``examples/workflow_api/condor-grid-example/`` directory
for working/usable code.

.. _guide-htcondor-anatomy:

Anatomy of the HTCondor Service
===============================

The in-simulation implementation of HTCondor in WRENCH is simplified in
terms of its functionality and design when compared to the actual
implementation of HTCondor. The :cpp:class:`wrench::HTCondorComputeService`
spawns two additional services during execution,
:cpp:class:`wrench::HTCondorCentralManagerService` and
:cpp:class:`wrench::HTCondorNegotiatorService`, both of which loosely correspond
to actual HTCondor daemons (``collector``, ``negotiator``, ``schedd``).
Their use is fully automated and transparent to the WRENCH developer.

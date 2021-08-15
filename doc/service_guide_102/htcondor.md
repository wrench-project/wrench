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
arguments to service constructors are ommitted for brevity (see the WMS
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




XXXXXXXXXXXXX END BARF




# Anatomy of the HTCondor Service #        {#guide-htcondor-anatomy}

The in-simulation implementation of HTCondor in WRENCH is simplified in
terms of its functionality and design when compare to the actual
implementation of HTCondor. The `wrench::HTCondorComputeService` spawns two
additional services during execution,
`wrench::HTCondorCentralManagerService` and
`wrench::HTCondorNegotiatorService`, both of which loosely correspond to
actual HTCondor daemons (`collector`, `negotiator`, `schedd`).


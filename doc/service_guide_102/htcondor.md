Interacting with a HTCondor compute service {#guide-102-htcondor}
============

**This section of the documentation is a work in progress**

WRENCH HTCondor service implementation spawns two additional services during 
execution: wrench::HTCondorCentralManagerService and wrench::HTCondorNegotiatorService.

The wrench::HTCondorCentralManagerService coordinates the execution of jobs
submitted to the HTCondor pool. Jobs submitted to the wrench::HTCondorComputeService
are then queued in a `std::vector<wrench::StandardJob *>`, which are then 
consumed as resources become available. The Central Manager also spawns the
execution of the wrench::HTCondorNegotiatorService, which performs matchmaking
between jobs and compute resources available in the pool. Note that job submission
in HTCondor is asynchronous, thus our simulated services operates independent 
from each other 



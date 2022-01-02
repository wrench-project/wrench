## WRENCH Examples

The `examples` directory in the WRENCH distribution contains several
example simulators that showcase the use of the WRENCH APIs.  

To build the examples just type `make examples` in the build directory. 


Examples are in two
categories and sub-directories:

  - `action_api`: example simulators that simulate the execution of arbitrary jobs that each consist of one or more arbitrary actions.
  - `workflow_api`: example simulators that simulate
 the execution of a workflow by a custom Workflow Management System
 (implemented as an Execution Controller using the WRENCH developer API). 
    There are many more examples here for now as this is the historical, but less generic, API. Looking at this
    example is still relevant if one plans to use the action API. 

Below are high-level descriptions of the example in each sub-directory.
Details on the specifics of each simulator are in extensive source code
 comments.
 
---

### Action-API simulator examples

  - `action_api/multi-action-multi-job`: A simulation of the execution of 3 jobs, each of which consisting of one or more actions, 
on a platform with a cloud compute service, a bare-metal compute service, and two storage services. There are dependencies between actions 
    within jobs, and inter-job dependencies. This example showcases the basic use of the generic action API. 
    
  - `action_api/job-action-failure`: An example similar to the previous one, but that includes actions that fail. The example shows how
to analyze action failures.
    
  - `action_api/super-custom-action`: An example that showcases how a custom action within a job can do powerful things (and essentially act itself
    as an execution controller).

  - `action_api/bare-metal-bag-of-tasks`: A simulation of the execution of a
  bag-of-tasks workload on a bare-metal compute
  service, with all data being read/written from/to a single
  storage service. Two  tasks are executed concurrently on
  the compute service until the workload is complete. 
  
### Workflow-API simulator examples

#### Simulators that showcase fundamental functionality (using the bare-metal compute service)

  - `workflow_api/basic-examples/bare-metal-chain`: A simulation of the execution of a
    chain workflow by a Workflow Management System on a bare-metal compute service,
    with all workflow data being read/written from/to a single storage
    service. The compute  service runs on a 10-core host, and each task is
    executed as a single job  that uses 10 cores

  - `workflow_api/basic-examples/bare-metal-chain-scratch`: Similar to the previous
    simulator, but the compute service now
    has scratch space to hold intermediate workflow files. Since files
    created in the scratch space during a job's execution are erased after
    that job's completion, the workflow is executed as a single multi-task
    job.

  

  - `workflow_api/basic-examples/bare-metal-complex-job`: A simulation of the execution of a
    one-task workflow on a compute service as a job that includes not only
    the task computation but also data movements.
    
  - `workflow_api/basic-examples/bare-metal-data-movement`: A simulation that is similar
    to the previous one, but instead  of using a complex job to do data movements
    and deletions, the WMS does it "by hand".


#### Workflow-API Simulators that showcase the use of the cloud compute service

  - `workflow_api/basic-examples/cloud-bag-of-tasks`: A simulation of the execution of a
       bag-of-tasks workflow by a Workflow Management System on a cloud compute
       service, with all workflow data being read/written from/to a single
       storage service. Up to two workflow tasks are executed concurrently on
       the compute service, in which case one task is executed on 6 cores and
       the other on 4 cores.
       
  - `workflow_api/workflow_api/basic-examples/cloud-bag-of-tasks-energy`: A simulation identical to the previous one, 
      but that showcases energy consumption simulation functionality. 

       
#### Workflow-API Simulators that showcase the use of the batch compute service

  - `workflow_api/basic-examples/batch-bag-of-tasks`: A simulation of the execution of a
      bag-of-tasks workflow by a Workflow Management System on a batch compute
      service, with all workflow data being read/written from/to a single
      storage service. Up to two workflow tasks are executed concurrently on
      the compute service on  two compute nodes, each of them using 10 cores.
      **This example also features low-level workflow execution handling, and
      dealing with job failures**.
      
  - `workflow_api/basic-examples/batch-pilot-jobs`: A simulation that showcases the use of 
    "pilot jobs" on a batch compute service. In this example, due to a pilot job
    expiration, the workflow  does not complete successfully.
    
#### Workflow-API Examples with real workflows and more sophisticated WMS implementations

  - `workflow_api/real-workflow-example`: Two simulators, one in which the workflow is executed
     using a batch compute service, and another in which the workflow is executed
     using a cloud compute service. These simulators take as input workflow description
     files from real-world workflow applications. They use the scheduler abstraction
     provided by WRENCH to implement complex Workflow Management System. 
     
---

## WRENCH Examples

The `examples` directory in the WRENCH distribution contains several
example simulators that showcase the use of the WRENCH APIs.  Each
simulator (implemented using the WRENCH user API) simulates
 the execution of a workflow by a custom Workflow Management System
 (implemented using the WRENCH developer API).

Below are high-level descriptions of the example in each sub-directory.
Details on the specifics of each simulator are in extensive source code
 comments.
 
---

### Basic simulator examples

#### Simulators that showcase fundamental functionality (using the bare-metal compute service)

  - `basic-examples/bare-metal-chain`: A simulation of the execution of a
    chain workflow by a Workflow Management System on a bare-metal compute service,
    with all workflow data being read/written from/to a single storage
    service. The compute  service runs on a 10-core host, and each task is
    executed as a single job  that uses 10 cores

  - `basic-examples/bare-metal-chain-scratch`: Similar to the previous
    simulator, but the compute service now
    has scratch space to hold intermediate workflow files. Since files
    created in the scratch space during a job's execution are erased after
    that job's completion, the workflow is executed as a single multi-task
    job.

  - `basic-examples/bare-metal-bag-of-tasks`: A simulation of the execution of a
     bag-of-tasks workflow by a Workflow Management System on a bare-metal compute
     service, with all workflow data being read/written from/to a single
     storage service. Up to two workflow tasks are executed concurrently on
     the compute service, in which case one task is executed on 6 cores and
     the other on 4 cores.

  - `basic-examples/bare-metal-complex-job`: A simulation of the execution of a
    one-task workflow on a compute service as a job that includes not only
    the task computation but also data movements.
    
  - `basic-examples/bare-metal-data-movement`: A simulation that is similar
    to the previous one, but instead  of using a complex job to do data movements
    and deletions, the WMS does it "by hand".


#### Simulators that showcase the use of the cloud compute service

  - `basic-examples/cloud-bag-of-tasks`: A simulation of the execution of a
       bag-of-tasks workflow by a Workflow Management System on a cloud compute
       service, with all workflow data being read/written from/to a single
       storage service. Up to two workflow tasks are executed concurrently on
       the compute service, in which case one task is executed on 6 cores and
       the other on 4 cores.
       
  - `basic-examples/cloud-bag-of-tasks-energy`: A simulation identical to the previous one, 
      but that showcases energy consumption simulation functionality. 

       
#### Simulators that showcase the use of the batch compute service

  - `basic-examples/batch-bag-of-tasks`: A simulation of the execution of a
      bag-of-tasks workflow by a Workflow Management System on a batch compute
      service, with all workflow data being read/written from/to a single
      storage service. Up to two workflow tasks are executed concurrently on
      the compute service on  two compute nodes, each of them using 10 cores.
      **This example also features low-level workflow execution handling, and
      dealing with job failures**.
      
  - `basic-examples/batch-pilot-jobs`: A simulation that showcases the use of 
    "pilot jobs" on a batch compute service. In this example, due to a pilot job
    expiration, the workflow  does not complete successfully.
      
---

### Examples with real workflows and more sophisticated WMS implementations

  - `real-workflow-example`: Two simulators, one in which the workflow is executed
     using a batch compute service, and another in which the workflow is executed
     using a cloud compute service. These simulators take as input workflow description
     files from real-world workflow applications. They use the scheduler abstraction
     provided by WRENCH to implement complex Workflow Management System. 
     
---

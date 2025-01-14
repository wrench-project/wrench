# API draft

This document outlines thoughts and design choices regarding the implementation
of a serverless feature in WRENCH.

## A new service: `ServerlessComputeService`

### Overview

This is a compute service in charge of a serverless infrastructure. It
implements almost none of the standard `ComputeService` methods (throwing a
"not implemented" exception in them, as done for instance quite a bit by
the `CloudComputeService`) because it doesn't support any job submission.
Most of its methods are private, and called via the `FunctionManager`
service (see next section).  Its constructor should specify:
  - a set of compute hosts each with a disk attached to it (for now, all homogeneous)
  - "# slots" per compute host that defines the maximum number of concurrent function invocations (may be more sophisticated in the future)
  - Time limit for function invocations
  - Size of local storage for each invocation on a compute host (AWS's default is 512MB)
  - A storage service at each compute host that will hold all images and provide local storage to running containers (the two compete with each other, with priority given to running containers)
  - Various properties that define behaviors, overheads, etc.

Upon startup, the service creates an "internal" bare-metal compute service and associated storage service on each compute host from the get go. Whenever a function is invoked, a custom action is created and submitted to one of the bare-metal service. The service keeps track of downloaded (and being downloaded) images for each compute host. 

### Issues and Thoughts

  - Concurrency: we will need to handle concurrent requests rationally (don't start a download of an image that's already being downloaded)

  - Node allocation policy: 
    - right now, we'll hardcode something, but it's not trivial
      - Say we have 2 2-slot compute nodes and we submit 2 function invocations... what do we do? 
    - eventually perhaps the user can pass lambdas for customizing behavior? (perhaps beyond the scope of this ICS496 project)

  - What about storage?
    - Ideally there would be a short-lived storage service created for each invocation, so that each invocation has its own playpen
    - Henri believes that this can be done with the current abstractions provided by WRENCH:
      - The "internal" storage service on the compute node is set to use LRU caching
      - It is where downloaded images are stored
      - Whenever a function is invoked: 
        - Download the corresponding image if needed
        - Read the corresponding image from the disk (while it's being read, it's unevictable)
        - Create an unevictable (i.e., opened!) file of the container's size
        - Create a short-lived disk of that same size attached to the host
        - Start a storage service on that disk
        - Run the function
        - Terminate the storage service and delete the disk
        - Close and delete the unevictable file


### API draft

  - `void registerFunction(Function)`
    - Just registers a function

  - `void InvokeFunction(Function, FunctionInput)`
    - Sends a message and waits for a message back
      - Failure answers:
		- function not registered
		- can't admit the function call right now
	  - If success, another message will come later:
		- Completion
		- Timeout
		- Failure (during the execution)


## A new service: `FunctionManager`

### Overview

Just like the `JobManager` or `DataManager`, but for functions, with some twists. 

  - `static std::shared_ptr<Function> FunctionManager:createFunction( "name", std::shared_ptr<FunctionOutput> lambda( std::shared_ptr<FunctionInput> input, std::shared_ptr<StorateService> ss), FileLocation image)`
    - Create the notion of a function

  - `std::shared_ptr<Function> FunctionManager:register(std::vector<ServerlessComputeService> providers)`
    - Registers a function at one or more providers

  - `FunctionInvocation FunctionManager::invokeFunction(ServerlessComputeService, Function, FunctionInput)` 
    - Places a function invocation

  - `FunctionInvocation::is_running()`, `FunctionInvocation::is_done()`, `FunctionOutput FunctionInvocation::get_output()`
    - State finding methods

  - `FunctionInvocation::wait_one(one)`, `FunctionInvocation::wait_any(one)`, `FunctionInvocation::wait_all(list)`
    - These are a bit annoying to implement.  Basically we need to implement some notion of "wait request" that the `FunctionManager` keeps track of. On each function completion/failure, it may trigger a response to one or more request.  These functions return an array of states (completed, failed, timeout)? or perhaps the user has then to go through an inspect states on their own...

	

		



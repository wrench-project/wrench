# API draft

This document outlines thoughts and design choices regarding the implementation
of a serverless feature in WRENCH

## A new service: `ServerlessComputeService`

### Overview

This is a compute service in charge of a serverless infrastructure. It
implements almost none of the standard `ComputeService` methods (throwing
an "not implemented" exception in them) because it doesn't support any job. Most of its methods are
private, and called via the `FunctionManager` service (see next section).
Its constructor should specify:
  - a set of compute hosts (for now, all homogeneous)
  - "# slots" per compute host (may be super sophisticated in the future)
  - size of local storage for each invocation on a compute host (amazon's default is 512MB)
  - a storage service at each compute host that will hold all images and provide local storage to running containers (the two compete with each other, with priority given to running containers)
  - various overhead properties (e.g., the hot start feature is just some overhead that happens or not)

Upon startup, the service creates an "internal" bare-metal compute service on each host from the get go, and whenever a function is invoked, a custom action is created and submitted to one of the bare-metal service. 

### Issues and Thoughts

  - Concurrency: we will need to handle concurrent requests rationally (don't start a download of an image that's already being downloaded)

  - Node allocation policy: 
    - right now, we'll hardcode something, but it's not trivial
      - Say we have 2 2-slot compute nodes and we submit 2 function invocations... what do we do? 
    - eventually perhaps the user can pass lambdas for customizing behavior? (likely beyond the scope of this ICS496 project)

  - Difficult one: What about storage?
    - Ideally there would be a short-lived storage service created for each invocation, so that each invocation has its own playpen
    - Currently WRENCH does not allow the creation of a storage service with some specific size (it always grabs the whole disk), so perhaps a feature request to wrench that Henri can implement in a second
    - Furthermore, if we create a short-lived storage service for some function invocation, then we need to account for the fact that there is less available storage for caching images....
    - But then the ServerlessComputeService needs to keep track of storage manually, raising the question: how do we limit the storage used by a particular function invocation
    - One (ugly) solution would be to create a disk on the fly, which SimGrid allows. But then we need to remove storage capacity from the disk that stores images, which is terrible....
    - Bottom line:
      - We need to have a storage service with limited capacity for each function invocation, just so that the function, when it runs, will be isolated from everything and will get expected "out of storage" errors if it goes over its capacity
      - But then, how do we deal with "when a function is running, we have less space to store images, and in fact, we may need to evict images upon a function starting...."
      - It seems our current storage abstractions are not powerful enough



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

	

		



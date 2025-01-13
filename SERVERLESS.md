# API draft

## A new service: `ServerlessComputeService`

### Overview

This is a compute service for simulating a serverless infrastructure. It
implements almost none of the standard `ComputeService` methods (throwing
an "not implemented" exception in them) because it doesn't support any job. Most of its methods are
private, and called via the `FunctionManager` service (see next section).
Its constructor should specify:
  - "# slots" per machine (may be super sophisticated in the future)
  - size of local storage for each invocation on a compute node (amazon's default is 512MB)
  - a storage service that will hold all images and running containers (the two compete with each other, with priority given to running containers)
  - overhead properties (e.g., the hot start feature is just some overhead that happens or not)

It creates an internal bare-metal compute service on each host from the get go, and whenever a function is invoked, a custom action is created and submitted to on of the bare-metal service. 

### Issues and Thoughts

  - Concurrency: we will need to handle concurrent requests rationally (don't start a download of an image that's already being downloaded)

  - Node allocation policy: 
    - right now, we'll hardcode something, but it's not trivial
      - Say we have 2 2-slot compute nodes and we submit 2 function invocations... what do we do? 
    - eventually perhaps the user can pass lambdas for customizing behavior? (likely beyond the scope of this ICS496 project)

  - What about storage?
    - There should be a short-lived storage service created for each invocation
    - Currently WRENCH does not allow the creation of a storage service with some specific size (it always grabs the whole disk), so perhaps a feature request to wrench that Henri can implement in a second
    - But then the ServerlessComputeService needs to keep track of storage manually, which is perhaps ok


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

	

		



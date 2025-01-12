# API draft

## A New WRENCH service: `ServerlessComputeService`

### Overview

This is a compute service for simulating a serverless infrastructure. It
implements almost none of the standard `ComputeService` methods (throwing
an exception) because it doesn't support any job. Most of its methods are
private, and called via the `FunctionManager` service (see next section).
Its constructor should specify:
		- specifies "# slots" per machine
			- may be super sophisticated in the future
		- specifies size of local storage for each invocation at
		   head node (amazon's default is 512MB)
		- the read of ram can be used for caching images at a node
		- the two compete, with priority given to the function invocation disk space
		- Some overhead properties (e.g., the hot start feature is just some overhead that happens or not)


It creates an internal bare-metal compute service on each host from the get go. And then on each function invocation it:
  - Create an internal short-lived storage service (amusingly, I don't think that WRENCH allows to create a storage service with some specific size, it always grabs the whole disk... an easy fix likely.... not sure)
  - Create a custom action and submit it to one of the bare-metal services

### Issues and Thoughts

  - minor difficulty: it will need to handle concurrent requests (don't start a download of an image that's already being downloaded)

  - node allocation policy: right now, we'll hardcore something perhaps, but eventually perhaps the user can pass lambdas for customizing behavior? (likely beyond the scope of this ICS496 project)

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



## A new server: `FunctionManager`

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

	

		



.. _guide-102-serverless:

Interacting with a serverless compute service
==============================================


A serverless service provides simply user-level mechanisms to register and
invoke functions, all of which is done via a
:cpp:class:`wrench::FunctionManager`.

Here is an example interaction with a :cpp:class:`wrench::ServerlessComputeService`:

.. code:: cpp

   std:shared_ptr<wrench::ServerlessComputeService> some_serverless_cs;

   // Start a function manager
   auto function_manager = this->createFunctionManager();

   // Define the code of a function as a lambda, that takes in some user-specified
   // input object and a storage service that will be allocated specifically for each function execution, and
   // that produces some user-specified output object
   auto function_code = [](const std::shared_ptr<FunctionInput>& input,
                           const std::shared_ptr<StorageService>& my_storage_service) -> std::shared_ptr<FunctionOutput> {
               // This function just sleeps 10 seconds, but it could do absolutely anything
               Simulation::sleep(10);
               return std::make_shared<FunctionOutput>();
           };

   // Create an image file on some storage service (e.g., the Docker image for the function environment)
   auto image_file = Simulation::addFile("image_file", 250 * MB);
   auto image_file_location = FileLocation::LOCATION(some_storage_service, image_file);
   StorageService::createFileAtLocation(image_file_location);

   // Create the function object
   auto function = wrench::FunctionManager::createFunction("my_function", function_code, image_file_location);

   // Define execution limits for the function execution
   double time_limit = 60.0;
   sg_size_t disk_space_limit_in_bytes = 500 * 1000 * 1000;
   sg_size_t RAM_limit_in_bytes = 200 * 1000 * 1000;
   sg_size_t ingress_in_bytes = 30 * 1000 * 1000;
   sg_size_t egress_in_bytes = 40 * 1000 * 1000;

   // Register the function to the serverless compute service, via the function manager
   auto registered_function = function_manager->registerFunction(function, compute_service,
                                                                 time_limit,
                                                                 disk_space_limit_in_bytes,
                                                                 RAM_limit_in_bytes,
                                                                 ingress_in_bytes,
                                                                 egress_in_bytes);

   // Invoke the function
   auto inv = function_manager->invokeFunction(registered_function, compute_service,
                                               function_input);

   // Wait for the invocation to complete
   function_manager->wait_one(inv);

   // Retrieve information, output, etc.
   auto function_output = inv->getOutput();
   double start_date = inv->getStartDate();


Note that the serverless compute service will decide on which physical resources
function invocations are placed. The underlying physical resources are
completely hidden.

See the execution controller implementation in
``examples/serverless_api/basic/ServerlessExampleExecutionController.cpp``
for a more complete example.

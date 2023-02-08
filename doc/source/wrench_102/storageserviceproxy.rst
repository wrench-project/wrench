.. _guide-102-storageserviceproxy:

StorageServiceProxy
=====================================

Sometimes you dont want a host to talk directly to the :cpp:class:`wrench::StorageService` that has a file.

Perhapse you have a cache, or a local data manager that you would prefer accesses the file on their behalf.

For this we have :cpp:class:`wrench::StorageServiceProxy` 
To create a :cpp:class:`wrench::StorageServiceProxy` use the :cpp:class:`wrench::StorageServiceProxy::createRedirectProxy`
This function takes a host to start on, a :cpp:class:`wrench::StorageService` to use as cache (this is assumed to be on the same host), and an optional Default remote :cpp:class:`wrench::StorageService` as well as the standard property and Message_payloads lists.

If proxy is given a default remote location, it can be used exactly like a normal storage service, it will simply use the cache and default remote file server.

If no default location is given, or the file is on a different remote :cpp:class:`wrench::StorageService` either :cpp:class:`wrench::StorageServiceProxy::readFile(wrench::StorageService,wrench::DataFile)` must be used, or the :cpp:class:`wrench::FileLocation` used to locate the file must be a :cpp:class:`wrench::ProxyLocation.`

:cpp:class:`wrench::ProxyLocation` has the same factories as a normal location, except they take an extra :cpp:class:`wrench::StorageService` `target` to use as a remote :cpp:class:`wrench::StorageService`.  There is also a factory that takes any existing location and the `target`.  
For this proxy location `ss` should be the proxy to access.


.. code:: cpp

   std::shared_ptr<wrench::StorageService> cache;
   std::shared_ptr<wrench::StorageService> remote;
   std::shared_ptr<wrench::StorageService> default;
   std::shared_ptr<wrench::StorageServiceProxy> proxy;

   [...]
   //create all StorageServices
   remote = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Remote", ...));
   cache = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Proxy", ...));
   default = simulation->add(wrench::SimpleStorageService::createSimpleStorageService("Default", ...));
   
   //create a Proxy using the cache and default as a default
   proxy=simulation->add(
			wrench::StorageServiceProxy::createRedirectProxy(
		    	"Proxy", 
		    	cache,
		    	remote,
		    	{{wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD,"CopyThenRead"}},
		    	{}
	        )
   		);
   		
   	[...]
   	
   	proxy.readFile(someDataFile);//Checks the cache for someDataFile, if it does not exist, checks default
   	readFile(FileLocation::LOCATION(proxy,someDataFile));//same
   	
   	proxy.readFile(remote,someOtherDataFile);
   	readFile(ProxyFileLocation::LOCATION(remote,FileLocaiton::LOCATION(proxy, someOtherDataFile));//read the file from the cace, or remote, not default
   
   	


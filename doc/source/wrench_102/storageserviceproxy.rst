.. _guide-102-storageserviceproxy:

StorageServiceProxy
=====================================

Sometimes you dont want a host to talk directly to the :cpp:class:`wrench::StorageService` that has a file.

Perhapse you have a cache, or a local data manager that you would prefer accesses the file on behalf of the host.

For this we have :cpp:class:`wrench::StorageServiceProxy` 

#Creating a Proxy
To create a :cpp:class:`wrench::StorageServiceProxy` use the :cpp:class:`wrench::StorageServiceProxy::createRedirectProxy`
This function takes a host to start on, a :cpp:class:`wrench::StorageService` to use as cache (this is assumed to be on the same host), and an optional Default remote :cpp:class:`wrench::StorageService` as well as the standard property and Message_payloads lists.


.. code:: cpp

   std::shared_ptr<wrench::StorageService> cache;//the storage service to use as a cache
   std::shared_ptr<wrench::StorageService> default;//The storage server to be used as a remote default
   std::shared_ptr<wrench::StorageService> remote;//another potential remote storage server
   std::shared_ptr<wrench::StorageServiceProxy> proxy;//the proxy

   [...]
   
   //create all StorageServices as SimpleStorageService
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
The :cpp:class:`wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD` property is important.  Due to a limitation in SimGrid, a file can not be read while it is still being written to a disk.  As such, there is no efficient way to say "As the file is being copied to the cache, send the avaliable bytes to anyone waiting on the file."
Since this is the desired behavior of a cache, some work arrounds had to be implemented.  Each one has specific advantages and drawbacks, so consider which is best in your specific case.  After a file has entered the cache completely, there is no difference.
- CopyThenRead copies the file to the cache, and then reads the file from the cache to any waiting hosts.  This option offers the best file-to-cache time, and stresses the network the most realistically, but the file will arive "late" at the actual waiting clients.

- MagicRead assumes the time to read the file from the cache can be completely amortized while coping the file to the cache, and will copy the file to the cache, then magically send it to anyone waiting.  If this assumption is correct, Magic read allows the file-to-cache and time-to-host time to be accurate.  However, as this does stress the internal network as much.

- ReadThrough reads the file directly to the host and once the read has finished, it instantly creates the file on the cache.  This assumes the route between the host and the remote server goes through the proxy.  For single client reads, this is the best option offering the best file-to-host accuracy, and correct network stress.  However, the file-to-cache time is slower, and any additional clients waiting on the file will have to wait until cache gets it before reading.

|-------------|----------|---------|
||CopyThenRead|MagicRead|ReadThrough|
|-------------|-----------|---------|
|File-to-Cache Time|Accurate|Accurate|Overestimated|
|File-to-host time|Overestimated|Probiably Accurate| Accurate|
|Internal Network congestion|Accurate|Underestimated|Accurate|
|-------------|-----------|---------|

:cpp:class:`wrench::StorageServiceProxy` does not support :cpp:class:`wrench::StorageService::createFile` due to its ambiguiuty.  If you wish to create a file on the remote use `remote->createFile()`.  If you wish to create a file in the cache, use `cache->createFile()` or `proxy->getCache()->createFile()`

.. code:: cpp
	cache->createFile(someFile);//create a file on the cache
	proxy->getCache()->createFile(someOtherFile);//create a file on the cache
	
	remote->createFile(someFile);//create a file on the remote
	
	
#Using a Proxy
If proxy is given a default remote location, it can be used exactly like a normal storage service, it will simply use the cache and default remote file server.  
.. code:: cpp
	proxy->readFile(someDataFile);//Checks the cache for someDataFile, if it does not exist, checks default
   	readFile(FileLocation::LOCATION(proxy,someDataFile));//same, but presumably the file is now cached
	proxy->writeFile(someDataFile);//Write a file to the default remote and the cache
	
	
If no default location is given, or the file is on a different remote :cpp:class:`wrench::StorageService` either :cpp:class:`wrench::StorageServiceProxy::readFile(wrench::StorageService,wrench::DataFile)` must be used, or the :cpp:class:`wrench::FileLocation` used to locate the file must be a :cpp:class:`wrench::ProxyLocation.` 

:cpp:class:`wrench::ProxyLocation` has the same factories as a normal wrench::FileLocation`, except they take an extra :cpp:class:`wrench::StorageService` `target` to use as a remote :cpp:class:`wrench::StorageService`.  There is also a factory that takes any existing location and the `target`.  
For this proxy location `ss` should be the proxy to access.

.. code:: cpp
   	proxy->readFile(remote,someOtherDataFile);
   	readFile(ProxyFileLocation::LOCATION(
		remote,//target a location other than default
		FileLocaiton::LOCATION(//the expected location of the file
			proxy, //on the proxy
			someOtherDataFile
		)
	);//read the file from the cace, or remote, not default
    
	proxy.writeFile(remote,someDataFile);//Write a file to the remote

Proxys do not support file copy.  If you wish to copy a file to or from a proxy, use `proxy->getCache()` instead.  
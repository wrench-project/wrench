.. _guide-102-storageserviceproxy:

StorageServiceProxy
=====================================

There are cases in which a Controller may not want to talk directly to the :cpp:class:`wrench::StorageService`
that is known to have a particular file: there is a cache that should be used first in case
if happens to hold that file.  For making the simulation of such a scenario straightforward,
WRENCH provides a :cpp:class:`wrench::StorageServiceProxy` abstraction.

Creating a Proxy
-------------------------------------

To create a :cpp:class:`wrench::StorageServiceProxy` use the :cpp:class:`wrench::StorageServiceProxy::createRedirectProxy()` method,
which takes the name of a host on which the proxy should be running,
a :cpp:class:`wrench::StorageService` to use as **local cache** (this should be on the same host, otherwise simulation may not be accurate),
and an **optional default remote** :cpp:class:`wrench::StorageService`, as well as the usual property and message payloads lists.

.. code:: cpp

   std::shared_ptr<wrench::StorageService> cache;      // The storage service to use as a cache
   std::shared_ptr<wrench::StorageService> default;    // The storage server to be used as a remote default
   std::shared_ptr<wrench::StorageService> remote;     // Another potential remote storage server
   std::shared_ptr<wrench::StorageServiceProxy> proxy; // The proxy

   [...]

   // Create all storage services as SimpleStorageService
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

The :cpp:member:`wrench::StorageServiceProxyProperty::UNCACHED_READ_METHOD` property is important.
In WRENCH, at least for the time being, a file can not be read while it is still being written to a disk.
As such, there is no efficient way to say "As the file is being copied to the cache, send the available bytes to
anyone waiting on the file." Since this is the desired behavior of a cache, some work-arounds have been implemented.
Each one has specific advantages and drawbacks, so consider which is best in your specific case.
Once the file has been written/copied to the cache completely, there is no difference.

- *CopyThenRead* copies the file to the cache and then reads the file from the cache to any waiting hosts. This option offers the best file-to-cache time, and stresses the network the most realistically, but the file will arrive "late" at the actual waiting clients.

- *MagicRead* assumes the time to read the file from the cache can be completely amortized while coping the file to the cache, and will copy the file to the cache, then magically send it to anyone waiting. If this assumption is correct, MagicRead allows the file-to-cache and time-to-host time to be accurate.  However,  this does not stress the internal network as much as would be the case in reality.

- *ReadThrough* reads the file directly to the host and once the read has finished, it instantly creates the file on the cache.  This assumes the route between the host and the remote server goes through the proxy.For single client reads, this is the best option offering the best file-to-host accuracy, and correct network stress.  However, the file-to-cache time is slower, and any additional clients waiting on the file will have to wait until the ache gets it before reading.


.. list-table:: Comparison of the three UNCACHED_READ_METHOD options
   :widths: 25 25 25 25
   :header-rows: 1

   * - Scheme
     - File-to-Cache Time
     - File-to-Host Time
     - Internal Network Congestion
   * - **CopyThenRead**
     - Accurate
     - Accurate
     - Overestimated
   * - MagicRead
     - Overestimated
     - Probably Accurate
     - Accurate
   * - ReadThrough
     - Accurate
     - Underestimated
     - Accurate


:cpp:class:`wrench::StorageServiceProxy` does not support :cpp:class:`wrench::StorageService::createFile` due to it being ambiguous in the case of a proxy. If you wish to create a file on the remote use `remote->createFile()`.  If you wish to create a file in the cache, use `cache->createFile()` or `proxy->getCache()->createFile()`

.. code:: cpp

	cache->createFile(someFile);//create a file on the cache
	proxy->getCache()->createFile(someOtherFile);//create a file on the cache

	remote->createFile(someFile);//create a file on the remote


Using a Proxy
-------------------------------------

If proxy is given a default remote location, it can be used exactly like a normal storage service, it will simply use the cache
and default  to the default remote file server if the cache doesn't have the desired file.

.. code:: cpp

	proxy->readFile(someDataFile); // Checks the cache for someDataFile, if it does not exist, checks default
   	readFile(FileLocation::LOCATION(proxy,someDataFile)); // Same, but presumably the file is now cached
	proxy->writeFile(someDataFile); // Write a file to the default remote and the cache


If no default location is given, or the file is on a different remote :cpp:class:`wrench::StorageService` either :cpp:class:`wrench::StorageServiceProxy::readFile(wrench::StorageService,wrench::DataFile)` must be used, or the :cpp:class:`wrench::FileLocation` used to locate the file must be a :cpp:class:`wrench::ProxyLocation`.

:cpp:class:`wrench::ProxyLocation` has the same factories as a normal :cpp:class:`wrench::FileLocation`, except they take an extra :cpp:class:`wrench::StorageService` `target` to use as a remote :cpp:class:`wrench::StorageService`.  There is also a factory that takes any existing location and the `target`.
For this proxy location `ss` should be the proxy to access.

.. code:: cpp

   	proxy->readFile(remote,someOtherDataFile);
   	readFile(ProxyFileLocation::LOCATION(
		remote,//target a location other than default
		FileLocation::LOCATION(//the expected location of the file
			proxy, //on the proxy
			someOtherDataFile
		)
	);//read the file from the cace, or remote, not default

	proxy.writeFile(remote,someDataFile);//Write a file to the remote


Proxies do not support file copies for now, and copies have to be done directly using the underlying storage services.

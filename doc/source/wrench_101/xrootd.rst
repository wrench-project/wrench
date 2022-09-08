.. _guide-101-xrootd:

Creating an XRootD storage service
=================================

.. _guide-xrootd-overview:

Overview
========

An XRootD storage "service" is a representation of a distributed filesystem 
composed of multiple services arranged in a 64 tree.
Any node in the tree can read or delete any file stored on a storage node in its subtree.
Each storage node internaly uses a :ref:`Simple Storage Service <guide-101-simplestorage>`,
and given a specific storage node, all Simple Storage Service operations are avaliable.


.. _guide-xrootd-creating:

Creating an XRootD storage service
==================================

In WRENCH, an XRootD storage service abstractly represents a storage service
(:cpp:class:`wrench::StorageService`), which is defined by a tree of 
:cpp:class:`wrench::xrootd::Node` objects. 
An XRootD deployment requires an :cpp:class:`wrench::xrootd::XRootDDeployment` object to be created first.  
This deployment object manages the tree outside of simulation time.  An instance of an XRootDDeployment requires

-  The current simulation object;
   and
-  Optional Maps (``std::map``) of configurable default properties
   (:cpp:class:`wrench::xrootd::Property`) and configurable default message
   payloads (:cpp:class:`wrench::xrootd::MessagePayload`).

Once the deployment object is created, it can be used to instanciate Nodes.  First a Root Node must be instanciated with  
:cpp:funtion:wrench::xrootd::XRootDDeployment::createRootSupervisor.  This function requires

-	The hostname for the Root;
   and
-  Optional Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::xrootd::Property`) and configurable message
   payloads (:cpp:class:`wrench::xrootd::MessagePayload`).
   
Once the root has been instanciated, it can be used to build the rest of XRootD data federation tree.
Most nodes in the tree are supervisors, these nodes manage subtrees of other nodes and direct searches.
To create a new supervisor node use :cpp:funtion:wrench::xrootd::Node::createChildSupervisor on the 
node that will be the new node's parrent.
This function requires 

-	The hostname for the new node;
   and
-  Optional Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::xrootd::Property`) and configurable message
   payloads (:cpp:class:`wrench::xrootd::MessagePayload`).
   
Only Storage nodes can store files.  A storage node has an underlying ref:`Simple Storage Service <guide-101-simplestorage>`
that files are stored on.  Files can only be created on individual storage servers.
To create a storage node use :cpp:funtion:wrench::xrootd::Node::createChildStorageServer on the 
node that will be the new node's parrent.
This function requires 

-	The hostname for the new node;
-	The mount point to use;
   and
-  Optional Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::xrootd::Property`), configurable message
   payloads (:cpp:class:`wrench::xrootd::MessagePayload`);
   ,configurable properties for the underlying simple storage server
   (:cpp:class:`wrench::StorageServiceProperty`), and configurable message
   payloads for the underlying simple storage server (:cpp:class:`wrench::StorageServiceMessagePayload`)..
   

The example below creates a small XRootD deployment of 3 nodes, a root on host ``Root``, a supervisor node 
on host ``Super``, and a Storage node on ``Storage``. The services are arranged in a line as follows ``Root->Super->Storage``.
The XRootD deployment is configured to run a full search and cache lifetime of 1 hour, but ``Super`` only has a 30 minute cache.
The Storage server has ``/``.  Furthermore, the number
of maximum concurrent data connections supported by the internal storage service is
configured to be 8, and the message sent to the service to read a file is 1KiB:
It then creates ``someFile`` on ``Storage``

.. code:: cpp

	auto xrootd_deployment(simulation,
                                       {{wrench::XRootD::Property::CACHE_MAX_LIFETIME, "3600"},
                                        {wrench::XRootD::Property::REDUCED_SIMULATION, "false"}},
                                       {});
	auto root = xrootd_deployment.createRootSupervisor("Root");
    auto super = root->addChildSupervisor("super1",{wrench::XRootD::Property::CACHE_MAX_LIFETIME, "1800"});
    auto storage = super->addChildStorageServer("Storage", "/", 
    											{}, 
    											{},
    											{wrench::SimpleStorageProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "8"}},
                                          		{{wrench::SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, "1024"}
                                          	);
	storage->createFile(someFile);

See the documentation of :cpp:class:`wrench::xrootd::Property` and
:cpp:class:`wrench::xrootd::MessagePayload` for all possible
configuration options.

Also see the simulator in the ``examples/action_api/XRootD``
use an XRootD storage services.

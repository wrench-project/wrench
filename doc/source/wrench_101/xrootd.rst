.. _guide-101-XRootD:

Creating an XRootD storage service
=================================

.. _guide-xrootd-overview:

Overview
========

An XRootD storage service is a distributed file system that is
composed of individual storage services arranged in a tree of arity at most 64. There are two kinds of
nodes, *storage nodes* and *supervisor nodes*.
A storage node internally uses a :ref:`Simple Storage Service <guide-101-simplestorage>`
and supports all the Simple Storage Service operations. A supervisor node is always the root of a
sub-tree and can perform file searches in that sub-tree, all the while maintaining a cache
of recent file search results (with a time-to-live for remembering these results).


.. _guide-xrootd-creating:

Creating an XRootD storage service
==================================

An XRootD storage service is described by specifying the deployment of the nodes in the XRootD tree
on hardware resources (i.e., hosts where individual nodes in the
 tree will execute and access disks available at these hosts). To this end,
a :cpp:class:`wrench::XRootD::Deployment` object must be created first, before the
simulation is launched. An instance of an Deployment is constructed based on:

-  A :cpp:class:`wrench::Simulation` object;
-  Optional Maps (``std::map``) of configurable default properties
   (:cpp:class:`wrench::XRootD::Property`) and configurable default message
   payloads (:cpp:class:`wrench::XRootD::MessagePayload`).

Once the deployment object is created, it can be used to add nodes to the tree, i.e., instances
of the :cpp:class:`wrench::XRootD::Node` class.  First a root node must be instantiated by calling the
:cpp:class:`wrench::XRootD::Deployment::createRootSupervisor()` method.

Once the root node has been instantiated, it can be used to build the rest of XRootD tree. Some nodes
in the trees are *supervisors*, i.e., they know about all other nodes in the subtree of which they are the root and can direct
searches for files down this subtree. Creating a new supervisor node in the tree is simply done
by calling the :cpp:class:`wrench::XRootD::Node::addChildSupervisor()` method on the
node that will be the new node's parent.

The other kind of node is a *storage node*, which can store files.
A storage node has an underlying ref:`Simple Storage Service <guide-101-simplestorage>`
that stores the files.
Creating a storage node is done by calling the :cpp:class:`wrench::XRootD::Node::addChildStorageServer()` on the
node that will be the new node's parent.
   

The example below creates a small XRootD deployment of 3 nodes, a root on host ``Root``, a supervisor node 
on host ``Super``, and a Storage node on ``Storage``. The nodes are arranged in a tree of arity 1 as follows ``Root->Super->Storage``.
The XRootD deployment is configured to simulate all underlying communications involved during a search
(the ``REDUCED_SIMULATION`` property). The cache lifetime is at most to 1 hour (the ``CACHE_MAX_LIFETIME`` property), but
the supervisor running on ``Super`` has only a 30-minute cache lifetime. The cache is where a supervisor keeps
the locations of files that it has previous found via searches. The storage node running on
``Storage`` is created with parameters similar to that used to create a SimpleStorageService instance. In
this example, it has mountpoint ``/``, can support up to 8 concurrent data connections, and the size of the
control message that is sent to it to request a file read is 2KiB.  Finally, in this example, a copy of file
``someFile`` is created ab initio on the storage node.

.. code:: cpp

    wrench::XRootD::XRootDDeploment xrootd_deployment(simulation,
                               {{wrench::XRootD::Property::CACHE_MAX_LIFETIME, "3600"},
                               {wrench::XRootD::Property::REDUCED_SIMULATION, "false"}},
                               {});
    auto root = xrootd_deployment.createRootSupervisor("Root");
    auto super = root->addChildSupervisor("Super", {wrench::XRootD::Property::CACHE_MAX_LIFETIME, "1800"});
    auto storage = super->addChildStorageServer(
        "Storage", "/",
    	{},
    	{},
    	{{wrench::SimpleStorageProperty::MAX_NUM_CONCURRENT_DATA_CONNECTIONS, "8"}},
        {{wrench::SimpleStorageServiceMessagePayload::FILE_READ_REQUEST_MESSAGE_PAYLOAD, "2048"});

    storage->createFile(someFile);

See the documentation of :cpp:class:`wrench::XRootD::Property` and
:cpp:class:`wrench::XRootD::MessagePayload` for all possible
configuration options.

See the example simulator in the ``examples/action_api/XRootD`` directory for a more complex XRootD
deployment.

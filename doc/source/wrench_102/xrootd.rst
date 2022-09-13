.. _guide-102-xrootd:

Interacting with an XRootD deployment
=====================================


Recall that an XRootD deployment consists of a tree of instances
of :cpp:class:`wrench::xrootd::Node`, with some of these nodes 
being *supervisors nodes* and others being *storage nodes*. 
The following operations are supported by a supervisor node
(and are accomplished by the supervisor
interacting with the nodes that are in the subtree of which it is the root):

  -  Synchronously reading a file (rarely used by an execution controller but
     included for completeness); and 
  -  Semi-Synchronously deleting a file (execution waits for supervisor to acknowledge delete request, 
  but does not wait for the full XRootD subtree to be purged)

In addition, all storage nodes in an XRootD tree support all operations that an instance of :ref:`Simple Storage Service <guide-102-simplestorage>` does (which must be invoked directory on that node). 

All interactions above are done by calling member functions of
the :cpp:class:`wrench::StorageService` class. Some of these member functions
take an optional :cpp:class:`wrench::FileRegistryService` argument, in which case
they will also update entries in a file registry service (e.g., removing
an entry when a file is deleted).  

The following operations:

  - Writing to a file; and
  - Creating a file

are intentionally only implemented for storage nodes and not supervisor
nodes (i.e., subtrees), due to the ambiguity of which storage node in the
subtree rooted at the supervisor should storage the newly created data.


Several interactions with an XRootD Deployment are done simply by calling **virtual** methods of the :cpp:class:`wrench::StorageService` class, but it is also 
possible to call directly methods of these :ref:`Simple Storage Service <guide-102-simplestorage>` class for XRootD storage nodes. This is because, in the XRootD distributed
file systems, so notions (such as the location of a file) are different than in a non-distributed file system. For instance: 

.. code:: cpp

   std::shared_ptr<wrench::xrootd::Deployment> deployment;
   std::shared_ptr<wrench::DataFile> some_file;

   [...]

   // Read a file from one specific storage node 
   deployment->getRootSupervisor()->getChild(0)->readFile(some_file);

   // Delete a file from the whole subtree, which may
   // delete the file at multiple storage nodes
   deployment->getRootSupervisor()->deleteFile(some_file);
   

Note that file deletion from an XRootD (sub)tree  will not return an error
even if the file does not exist. This is because the delete operation is 
only semi-synchronous and XRootD does not
propagate "file not found" errors up the tree.  Similarly, the only indication that
a file read operation has failed is a network timeout while searching.


Note that reading and writing files is something an execution controller typically
does not do directly. Instead, jobs created by the execution controller contain
actions/tasks that read and write files as
they execute.  A XRootD supervisor node can then be passed to these tasks/actions 
exactly as one would pass a :ref:`Simple Storage Service <guide-102-simplestorage>` instance. For instance: 

.. code:: cpp

    // Create a job
    auto job = job_manager->createCompoundJob("some_job");
    // Add a file read action that will read from an XRootD supervisor node
    auto action = job->addFileReadAction("file_read", some_file, deployment->getRootSupervisor());
	

See the execution controller implementation in
``examples/action-api/XRootD/Controller.cpp``
for a more complete example.

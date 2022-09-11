.. _guide-102-xrootd:

Interacting with an XRootD deployment
==========================================


The following operations are supported by an supervisor instance of
:cpp:class:`wrench::xrootd::Node`: and will be done in the subtree that node supervises

-  Synchronously read a file (rarely used by an execution controller but
   included for completeness)
-  Semi-Synchronously delete a file (execution waits for initial Node to acknowledge delete request, 
but does not wait for the full XRootD subtree to be purged)
-  Get parrent or specific child of node (done outside simulation time)

In addition, all Storage Nodes support all operations that :ref:`Simple Storage Service <guide-102-simplestorage>` does,
but only when done directly on that node.

These interactions above are done by calling member functions of
the :erf:class:`wrench::StorageService` class. Some of these member functions
take an optional :cpp:class:`wrench::FileRegistryService` argument, in which case
they will also update entries in a file registry service (e.g., removing
an entry when a file is deleted).  
The operataions
- Write to file and
- Create File
are intentionaly only implimented for individual storage nodes and not subtrees
because of the inherent ambiguity of which Node in the subtree is being written too.


Several interactions with an XRootD Deployment are done simply 
 by calling **virtual** of the :cpp:class:`wrench::StorageService` class, although these
are generaly also possible by calling the same static methods as :ref:`Simple Storage Service <guide-102-simplestorage>`
some of the core concepts there (like knowing the location of the file before you try to read it)
break down for distributed file systems. These make
it possible to read and delete files. For instance:

.. code:: cpp

   std::shared_ptr<wrench::xrootd::Deployment> deployment;
   std::shared_ptr<wrench::DataFile> some_file;

   [...]
   // Read a file from the first subtree then delete it from the whole deployment
   
   deployment->getRootSupervisor()->getChild(0)->readFile(some_file);
   deployment->getRootSupervisor()->deleteFile(some_file);
   
Note: file deletion in XRootD will appear to work, even if the file does not exist as delete is semi-synchronous and XRootD does not
propogate file not found errors up the tree.  Similaraly, the only indication a readFile has failed is a network timeout while searching.
Reading and writing files is something an execution controller typically
does not do directly (instead, workflow tasks read and write files as
they execute).  The same file read API's that a developer would use for :ref:`Simple Storage Service <guide-102-simplestorage>`
work for xrootd subtrees (within the supported operations atleast).

.. code:: cpp

    auto job1 = job_manager->createCompoundJob("job1");
    auto fileread1 = job1->addFileReadAction("fileread1", some_file, deployment->getRootSupervisor());
	

See the execution controller implementation in
``examples/action-api/XRootD/Controller.cpp``
for a more complete example.

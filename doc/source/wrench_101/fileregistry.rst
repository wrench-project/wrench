.. _guide-101-fileregistry:

Creating a file registry service
================================

.. _guide-fileregistry-overview:

Overview
========

A file registry service is a simple store of key-values pairs where keys
are files (i.e., :cpp:class:`wrench::DataFile`) and values are file locations
(i.e., :cpp:class:`wrench::FileLocation`). It is used to keep track of the
location of file copies. In real-world deployments, this service is
often called a “replica catalog”.

.. _guide-fileregistry-creating:

Creating a file registry service
================================

In WRENCH, a file registry service is defined by the
:cpp:class:`wrench::FileRegistryService` class, an instantiation of which
requires the following parameters:

-  The name of a host on which to start the service; and
-  Maps (``std::map``) of configurable properties
   (:cpp:class:`wrench::NetworkProximityServiceProperty`) and configurable
   message payloads (:cpp:class:`wrench::NetworkProximityServiceMessagePayload`).

The example below creates an instance that runs on host
``ReplicaCatalog``. Furthermore, the service is configured so that
looking up an entry takes 100 flops of computation, and so that the
message sent to the service to lookup an entry is 1KiB:

.. code:: cpp

   auto fr_service = simulation->add(
             new wrench::FileRegistryService("ReplicaCatalog",
                                          {{wrench::FileRegistryServiceProperty::LOOKUP_COMPUTE_COST, "0.1"}},
                                          {{wrench::BareMetalComputeServiceMessagePayload::FILE_LOOKUP_REQUEST_MESSAGE_PAYLOAD, 1024}});

See the documentation of :cpp:class:`wrench::FileRegistryServiceProperty` and
:cpp:class:`wrench::FileRegistryServiceMessagePayload` for all possible
configuration options.

Guide                        {#guide}
============

<!--
@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/guide.html">Developer</a> - <a href="../internal/guide.html">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/guide.html">User</a> - <a href="../internal/guide.html">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/guide.html">User</a> -  <a href="../developer/guide.html">Developer</a></div> @endWRENCHDoc
-->


This page provides pointers to high-level information for WRENCH users who
want to build simulators. The examples provided  in the  ```example/```
directory are a great first start. This page goes further by detailing what
possibilities are available to users beyond what  is shown in the examples.

---

## Simulated Platform Hardware Configurations

A WRENCH simulator must specify the hardware configuration of the platform
to be simulator (i.e., compute resources, storage resources, network
resources). WRENCH does not do anything specific and accepts any valid
[SimGrid platform description in
XML](https://simgrid.org/doc/latest/platform.html).  The documentation is
extensive, as many options are possible. We strongly suggest users to
inspect the platform  files provided  with the  WRENCH examples (in
directory ```examples/simple-example/platform_files/```). These  files are
simply passed to the ```wrench::Simulation::instantiatePlatform()```
method.

---

## Simulated CyberInfrastructure Services

A WRENCH simulation consists in a WMS  (Workflow Management System) that interacts with CyberInfrastructure Service. Several such (simulated) services are implemented in WRENCH and can thus be instantiated on the simulation platform out-of-the-box.  The examples in the ```examples``` directory showcase the use of some of these services, but there are others! WRENCH currently provide the following CyberInfrastructure services:


1. Compute Services
    1. [Bare-metal Servers](@ref guide-baremetal)
    2. [Cloud Platforms](@ref guide-cloud)
    3. [Virtualized Cluster Platforms](@ref guide-virtualizedcluster)
    4. [Batch-scheduled Clusters](@ref guide-batch)
    5. [HTCondor Workload Management System](@ref guide-htcondor)
    
2. Storage Services
    1. [Simple](@ref guide-simplestorage)

3. File Registry Services
    1. [File Registry Service](@ref guide-fileregistry)

4. Network Proximity Services
    1. [Network Proximity Service](@ref guide-networkproximity)

---

## Simulation Output

The goal of any simulator is to generate output that describes a simulated
execution, the outcomes of which can be analyzed for whatever purposes. The
class ```wrench::SimulationOutput``` contains all relevant information
(once the simulation has completed). This information can be accessed via
this class' API (see the relevant documentation in the API Reference
section). Note that there are also  methods to configure the type  and amount of
output generated (see the
```wrench::SimulationOutput::enable*Timestamps()``` methods).

The simulation output can be exported to a JSON file,  using one of the
```wrench::SimulationOutput::dump*JSON()``` methods. See the documentation of each method to see the structure of the JSON output, in case  you want to parse/process the JSON yourself. Alternately, you  can use the 
**WRENCH dashboard** to open the JSON and gain access to interactive  visualization/inspection tools. To do so simple  run ```tools/wrench/wrench-dashboard```.




---


WRENCH Release Notes
------

### WRENCH 1.3

(_Jan 3, 2019_) This release includes a series of new features and bug fixes, including:

  - Development of a `wrench-init` tool
  - `MultiHostMultiCoreComputeService` re-implemented as `BareMetalComputeService`
  - Documentation of the JSON schema for workflows
  - Documentation Guide containing detailed descriptions of WRENCH core services
  - Bug fixes: #81

**Note**: WRENCH 1.2 requires [SimGrid 3.21](https://simgrid.org).

### WRENCH 1.2

(_Nov 6, 2018_) This release includes a series of new features and bug fixes, including:

  - New HTCondor compute service
  - Improved standard job submission to specific VM (Cloud Service)
  - Allow a WMS to start a new service dynamically
  - New function Simulation::dumpWorkflowExecutionJSON() for dumping the workflow execution data in a JSON format
  - Bug fixes: #67, #69, #79, #80

**Note**: WRENCH 1.2 requires [SimGrid 3.21](https://simgrid.org).

### WRENCH 1.1

(_Aug 26, 2018_) This release includes a series of **new features** and **bug fixes**, including:

  - Energy consumption by hosts and support for power state management
  - Virtual machine management support including shutdown, start, suspend, and resume operations
  - Enriched set of simulation events in the simulation output
  - Command-line options
  - Bug fixes: #38, #63, #64, #66
  - Code improvements: #36, #61

**Note**: WRENCH 1.1 requires [SimGrid 3.20](https://simgrid.org)

### WRENCH 1.0.1

(_Aug 14, 2018_) On this minor bug fix and small improvements release, we provide:

  - Ability to declare VM creation overhead in seconds to Cloud/VirtualizedCluster service
  - Ability to load batch workload trace file in JSON "batsim" format
  - Bug fixes: #51, #53, #54, #56, and #57

**Note**: WRENCH 1.0.1 requires [SimGrid 3.20](https://simgrid.org)

### WRENCH 1.0

(_Jun 16, 2018_) This release provides a set of (simulated) services that can be instantiated on the simulated platform:

  - Compute Services (multi-core multi-host, virtualized cluster, cloud computing, and batch computing)
  - Storage Service (including support for scratch space for computing nodes)
  - File Registry Service (file replica catalog)
  - Network Proximity Service (monitors the network and provide a database of host-to-host network distances)
  - Workflow Management Systems (WMSs)

**Note**: WRENCH 1.0 requires [SimGrid 3.20](https://simgrid.org)

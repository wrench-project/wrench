WRENCH Release Notes
------

# WRENCH 1.3

_Under development_


# WRENCH 1.2

This release includes a series of new features and bug fixes, including:

- #50: New HTCondor compute service
- #68: Improved standard job submission to specific VM (Cloud Service)
- #70: Allow a WMS to start a new service dynamically
- #74: New function Simulation::dumpWorkflowExecutionJSON() for dumping the workflow execution data in a JSON format
- Bug fixes: #67, #69, #79, #80

**Note**: WRENCH 1.2 requires [SimGrid 3.21](https://simgrid.org).


# WRENCH 1.1

This release includes a series of **new features** and **bug fixes**, including:

- #37: Energy consumption by hosts and support for power state management
- #60: Virtual machine management support including shutdown, start, suspend, and resume operations
- #13: Enriched set of simulation events in the simulation output
- #59: Command-line options
- Bug fixes: #38, #63, #64, #66
- Code improvements: #36, #61

**Note**: WRENCH 1.1 requires [SimGrid 3.20](https://simgrid.org) or higher.


# WRENCH 1.0.1

On this minor bug fix and small improvements release, we provide:

- #52: Ability to declare VM creation overhead in seconds to Cloud/VirtualizedCluster service
- #58: Ability to load batch workload trace file in JSON "batsim" format
- Bug fixes: #51, #53, #54, #56, and #57

**Note**: WRENCH 1.0.1 requires [SimGrid 3.20](https://simgrid.org/download.html) or higher.


# WRENCH 1.0

This release provides a set of (simulated) services that can be instantiated on the simulated platform:

- Compute Services (multi-core multi-host, virtualized cluster, cloud computing, and batch computing)
- Storage Service (including support for scratch space for computing nodes)
- File Registry Service (file replica catalog)
- Network Proximity Service (monitors the network and provide a database of host-to-host network distances)
- Workflow Management Systems (WMSs)

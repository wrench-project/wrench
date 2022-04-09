WRENCH Release Notes
------

### WRENCH 2.0

(April 8, 2022) This is a **major** release, which includes:

- Created a more general Developer API, called the *action API*, that makes it possible
  to simulate application workloads that are not necessarily workflow applications. Examples
  are provided in the `examples/action_api` directory. 
- Minor changes to the workflow API (which is now implemented internally on top of the action API).
- Removed support for the obsolete DAX XML workflow description format, which removes an external software dependency.
- Added support for the [WfCommons](https://wfcommons.org) JSON workflow description format.
- Many additional API functionality, typically as requested by users.
- Scalability improvements both in terms of simulation time and simulation memory footprint.
- New and improved documentation.

**Note**: WRENCH 2.0 requires [SimGrid 3.31](https://simgrid.org)


### WRENCH 1.11

(February 25, 2022) This release includes only **minor changes** and **bug fixes**. This is the last
release of WRENCH version 1.x. WRENCH version 2.0 will be released soon, which will include
minor changes to the current "workflow" API, a new more generic "non-workflow" API, as well as
significantly decreased simulation time and memory footprint.

**Note**: WRENCH 1.11 requires [SimGrid 3.30](https://simgrid.org)

---

### WRENCH 1.10

(October 16, 2021) This release includes a series of **new features**, **enhancements**, and **bug fixes**, including:

- Support for programmatic platform description ([#228](https://github.com/wrench-project/wrench/issues/228))
- Enable a job to check locations for a file in some order of preference ([#229](https://github.com/wrench-project/wrench/issues/229))
- Enhancement ([#226](https://github.com/wrench-project/wrench/issues/226))

**Note**: WRENCH 1.10 requires [SimGrid 3.29](https://simgrid.org)

---

### WRENCH 1.9

(August 24, 2021) This release includes a series of **new features**, **enhancements**, and **bug fixes**, including:

- Performance improvement ([#221](https://github.com/wrench-project/wrench/issues/221))
- Updates to HTCondor component and API ([#220](https://github.com/wrench-project/wrench/issues/220))
- Enhancement ([#225](https://github.com/wrench-project/wrench/issues/225))

**Note**: WRENCH 1.9 requires [SimGrid 3.27](https://simgrid.org)

---

### WRENCH 1.8

(February 18, 2021) This release includes a series of **new features**, **enhancements**, and **bug fixes**, including:

- I/O simulation model that includes the key features of the Linux page cache ([#199](https://github.com/wrench-project/wrench/issues/199), [#202](https://github.com/wrench-project/wrench/issues/202), [#218](https://github.com/wrench-project/wrench/issues/218))  
- Extended HTCondor component model with the Grid Universe ([#161](https://github.com/wrench-project/wrench/issues/161))
- Improvements to the WRENCH Dashboard ([#170](https://github.com/wrench-project/wrench/issues/170), [#212](https://github.com/wrench-project/wrench/issues/212))
- Improvements to documentation ([#219](https://github.com/wrench-project/wrench/issues/219) and code ([#189](https://github.com/wrench-project/wrench/issues/189), [#214](https://github.com/wrench-project/wrench/issues/214))

**Note**: WRENCH 1.8 requires [SimGrid 3.26](https://simgrid.org)

---

### WRENCH 1.7

(September 18, 2020) This release includes a series of **new features**, **enhancements**, and **bug fixes**, including:

- Redesign of the WRENCH Dashboard: includes a number of graphs for visualizing the Gantt chart of the workflow execution, host utilization, network bandwidth usage, and energy consumption ([#171](https://github.com/wrench-project/wrench/issues/171), [#183](https://github.com/wrench-project/wrench/issues/183), [#184](https://github.com/wrench-project/wrench/issues/184), [#185](https://github.com/wrench-project/wrench/issues/185), [#186](https://github.com/wrench-project/wrench/issues/186), [#173](https://github.com/wrench-project/wrench/issues/173), [#195](https://github.com/wrench-project/wrench/issues/195))
- Improvements to `BareMetalComputeService`: load is now equally distributed among hosts ([#169](https://github.com/wrench-project/wrench/issues/169)), and the service provides an API equivalent to the `squeue` Slurm command ([#176](https://github.com/wrench-project/wrench/issues/176))
- Improvements to `CloudComputeService`: added a function to get the `ComputeService` for a VM based on its name ([#187](https://github.com/wrench-project/wrench/issues/187))
- Improvements to `VirtualizedClusterComputeService`: added a function to get the physical host ([#190](https://github.com/wrench-project/wrench/issues/190))
- Enabled support for capturing network link's usage during the simulation ([#182](https://github.com/wrench-project/wrench/issues/182)) 
- Improvements to Simulation JSON output: added disk read/write failures ([#167](https://github.com/wrench-project/wrench/issues/167)), and network link usage ([#182](https://github.com/wrench-project/wrench/issues/182))
- Added an exception handling for ensuring a link bandwidth in the platform file is not set to zero ([#181](https://github.com/wrench-project/wrench/issues/181))
- Bug fixes and small enhancements: [#168](https://github.com/wrench-project/wrench/issues/168), [#172](https://github.com/wrench-project/wrench/issues/172), [#174](https://github.com/wrench-project/wrench/issues/174), [#178](https://github.com/wrench-project/wrench/issues/178), [#180](https://github.com/wrench-project/wrench/issues/180), [#191](https://github.com/wrench-project/wrench/issues/191), [#192](https://github.com/wrench-project/wrench/issues/192), [#200](https://github.com/wrench-project/wrench/issues/200)

**Note**: WRENCH 1.7 requires [SimGrid 3.25](https://simgrid.org).

---

### WRENCH 1.6

(May 7, 2020) This release includes a series of **new features**, **enhancements**, and **bug fixes**, including:

- Refactored the WRENCH documentation: WRENCH 101 for users, and WRENCH 102 for developers ([#156](https://github.com/wrench-project/wrench/issues/156))
- New collection of examples provided with WRENCH distribution: over 10 examples of simulators ([#157](https://github.com/wrench-project/wrench/issues/157))
- Removed dependency to Lemon library: we now use Boost, which is already used by SimGrid ([#159](https://github.com/wrench-project/wrench/issues/159))
- Simulation logging in now disabled by default: can be enabled using `--wrench-full-log` ([#158](https://github.com/wrench-project/wrench/issues/158))
- Refactored the BatchComputeService class: includes a conservative backfilling algorithm for validation purposes ([#152](https://github.com/wrench-project/wrench/issues/152))
- Improvements to simulation output processing and JSON output: includes task1, host, disk I/O, and energy ([#154](https://github.com/wrench-project/wrench/issues/154), [#122](https://github.com/wrench-project/wrench/issues/122), [#129](https://github.com/wrench-project/wrench/issues/129), [#133](https://github.com/wrench-project/wrench/issues/133))
- Improvements to the WRENCH Dashboard: [#130](https://github.com/wrench-project/wrench/issues/130), [#136](https://github.com/wrench-project/wrench/issues/136), [#137](https://github.com/wrench-project/wrench/issues/137), [#139](https://github.com/wrench-project/wrench/issues/139), [#146](https://github.com/wrench-project/wrench/issues/146), [#147](https://github.com/wrench-project/wrench/issues/147), [#148](https://github.com/wrench-project/wrench/issues/148), [#164](https://github.com/wrench-project/wrench/issues/164)
- Bug fixes and small enhancements: [#110](https://github.com/wrench-project/wrench/issues/110), [#141](https://github.com/wrench-project/wrench/issues/141), [#143](https://github.com/wrench-project/wrench/issues/143), [#144](https://github.com/wrench-project/wrench/issues/144), [#145](https://github.com/wrench-project/wrench/issues/145), [#151](https://github.com/wrench-project/wrench/issues/151), [#153](https://github.com/wrench-project/wrench/issues/153), [#160](https://github.com/wrench-project/wrench/issues/160), [#162](https://github.com/wrench-project/wrench/issues/162), [#163](https://github.com/wrench-project/wrench/issues/163)

**Note**: WRENCH 1.6 requires [SimGrid 3.25](https://simgrid.org).

---

### WRENCH 1.5

(Feb 7, 2020) This release includes a series of new features and bug fixes, including:

- Simulation of failures
- WRENCH Dashboard
- Enabled Pilot Job Submission for HTCondor compute service
- Improved exception handling
- Added a `BatchComputeServiceProperty` to make it possible to ignore bogus job specifications in workload trace files
- Allow users to define `SIMGRID_INSTALL_PATH` for non-standard SimGrid installations
- Code performance improvements
- Bug fixes: [#104](https://github.com/wrench-project/wrench/issues/104), [#116](https://github.com/wrench-project/wrench/issues/116), [#118](https://github.com/wrench-project/wrench/pull/118)

**Note**: WRENCH 1.5 requires [SimGrid 3.25](https://simgrid.org).

---

### WRENCH 1.4

(Apr 22, 2019) This release includes a series of new features and bug fixes, including:

- Updated [Batsched](https://gitlab.inria.fr/batsim/batsched) support (support to new JSON-based protocol)
- Dump `pstate` and energy consumption data as JSON
- Adding support for `bytesRead`, `bytesWritten`, and `avgCPU` for workflows defined as JSON
- Adding `ComputeService::getTotalNumCores()` and `ComputeService::getTotalNumIdleCores()` 
- `wrench::BatchComputeService` should handle requested vs. real job run times when replaying traces
- Bug fixes: [#97](https://github.com/wrench-project/wrench/issues/97), [#99](https://github.com/wrench-project/wrench/issues/99), [#100](https://github.com/wrench-project/wrench/issues/100)

**Note**: WRENCH 1.4 requires [SimGrid 3.21](https://simgrid.org).

---

### WRENCH 1.3

(_Jan 3, 2019_) This release includes a series of new features and bug fixes, including:

- Development of a `wrench-init` tool
- `MultiHostMultiCoreComputeService` re-implemented as `BareMetalComputeService`
- Documentation of the JSON schema for workflows
- Documentation Guide containing detailed descriptions of WRENCH core services
- Bug fixes: [#81](https://github.com/wrench-project/wrench/issues/81)

**Note**: WRENCH 1.3 requires [SimGrid 3.21](https://simgrid.org).

---

### WRENCH 1.2

(_Nov 6, 2018_) This release includes a series of new features and bug fixes, including:

- New HTCondor compute service
- Improved standard job submission to specific VM (Cloud Service)
- Allow a WMS to start a new service dynamically
- New function Simulation::dumpWorkflowExecutionJSON() for dumping the workflow execution data in a JSON format
- Bug fixes: #67, #69, #79, #80

**Note**: WRENCH 1.2 requires [SimGrid 3.21](https://simgrid.org).

---

### WRENCH 1.1

(_Aug 26, 2018_) This release includes a series of **new features** and **bug fixes**, including:

- Energy consumption by hosts and support for power state management
- Virtual machine management support including shutdown, start, suspend, and resume operations
- Enriched set of simulation events in the simulation output
- Command-line options
- Bug fixes: #38, #63, #64, #66
- Code improvements: #36, #61

**Note**: WRENCH 1.1 requires [SimGrid 3.20](https://simgrid.org)

---

### WRENCH 1.0.1

(_Aug 14, 2018_) On this minor bug fix and small improvements release, we provide:

- Ability to declare VM creation overhead in seconds to Cloud/VirtualizedCluster service
- Ability to load batch workload trace file in JSON "batsim" format
- Bug fixes: #51, #53, #54, #56, and #57

**Note**: WRENCH 1.0.1 requires [SimGrid 3.20](https://simgrid.org)

---

### WRENCH 1.0

(_Jun 16, 2018_) This release provides a set of (simulated) services that can be instantiated on the simulated platform:

- Compute Services (multi-core multi-host, virtualized cluster, cloud computing, and batch computing)
- Storage Service (including support for scratch space for computing nodes)
- File Registry Service (file replica catalog)
- Network Proximity Service (monitors the network and provide a database of host-to-host network distances)
- Workflow Management Systems (WMSs)

**Note**: WRENCH 1.0 requires [SimGrid 3.20](https://simgrid.org)

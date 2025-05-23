WRENCH Release Notes
------

### wrench 2.7-dev

(May 19, 2025) This release includes **a new API for simulating serverless infrastructures**, as well as **minor enhancements and updates**. More specifically:

- Implementation of a `ServerlessComputeService` to simulate serverless, function-based cloud infrastructures (e.g., AWS Lambda, Google Functions, etc). Comes with a simple user-level API and a `FunctionManager` component to develop simulation controllers that place function invocations.  An example is provided in the `examples` directory. Although several unit tests have been developed, this serverless simulation feature is still experimental. It will likely evolved, along with its API, in the near future.
- Removed all usage of `httplib` in wrench-daemon (which now uses only CrowCPP)
- Minor bug fixes and code/documentation cleanups

### wrench 2.6

(March 13, 2025) This release includes **minor enhancements and updates**, and, more importantly, an **upgrade to SimGrid 4**. More specifically:

- Implementation of a "backfilling depth" feature for EASY and conservative_bf batch scheduling algorithms
- Upgrade to SimGrid v4.0 and FSMod v0.3
- Minor code/documentation cleanups

### wrench 2.5

(December 16, 2024) This release includes **a new batch scheduling algorithm** and 
**minor enhancements and updates**. More specifically:

- Implementation of the EASY batch scheduling algorithm in BatchComputeService 
- New command-line argument for the wrench-daemon to specify the number of commports
- Minor code/documentation cleanups

### wrench 2.4

(October 29, 2024) this release includes **the use of FSMod for all file system simulation**, as well as 
**minor enhancements and updates**. More specifically:

- Removal of all file system simulation code, which was replaced by calls to
  the [SimGrid File System Module (FSMod)](https://github.com/simgrid/file-system-module), which is now a new
  software dependency for WRENCH
- API change: all numbers of bytes (file and memory sizes) are now of type `sg_size_t` instead of `double` (due to the use of FSMod above)
- Added REST API functionality and updated all documentation
- Minor code/documentation cleanups

### wrench 2.3

(September 10, 2024) this release includes **minor enhancements and upgrades**. More specifically:

- More full-feature REST API that gives access to the Action API in addition to the Workflow API
- Upgrade to SimGrid v3.36, which comes with several bug fixes
- Upgrade to WfCommon's WfFormat 1.5
- Countless minor bug fixes and code updates

### wrench 2.2

(July 20, 2023) this release includes **a new REST API**, **new StorageService implementations**, and **fast simulation of zero-size messages**. More specifically:

- Implementation of `wrench-daemon`, which can be started on the local machine and supports a REST API so that users can create and run simulations in a language-agnostic manner.
- Implementation of non-bufferized (i.e., buffer size of zero) storage services, which is transparent to the user but can vastly reduce simulation time by using a fluid (rather than message-based) model for how storage services read/write data to/from disk while sending/receiving that same data to/from the network. 
- API change by which a `FileLocation` now includes a `DataFile`.
- Added a `CACHING_BEHAVIOR` property to StorageService, which can take value `NONE` (the original behavior in which when full the storage service fails on writes) and `LRU` (the storage service implements a Least Recently Used strategy so as to function as a cache).
- Implementation of a File Proxy Service, which acts as a proxy for a file service while maintaining a local cache for files.
- Implementation of a Compound Storage Service, which acts as a proxy for an arbitrary set of Simple Storage Services and performs file striping.
- Implementation of an MPI action, which can be part of any job and makes it possible to simulate message-passing programs implemented with the MPI API. The simulation of the MPI program is handled by the SMPI component is SimGrid, which has proven both accurate and scalable. 
- Minor bug fixes and scalability improvements.

### wrench 2.1

(October 7, 2022) this release includes **a new storage service implementation**, **performance enhancements**, and **minor bug fixes**. More specifically:

- implementation of a new storage service for the simulation of the [XRootD](https://xrootd.slac.stanford.edu/) storage system, along with implementation and examples.
- performance and scalability improvements that reduce memory footprint and simulation execution time. 

**note**: wrench 2.0 requires [simgrid 3.32](https://simgrid.org)


### wrench 2.0

(April 8, 2022) this is a **major** release, which includes:

- created a more general developer api, called the *action api*, that makes it possible
  to simulate application workloads that are not necessarily workflow applications. examples
  are provided in the `examples/action_api` directory. 
- minor changes to the workflow api (which is now implemented internally on top of the action api).
- removed support for the obsolete dax XML workflow description format, which removes an external software dependency.
- added support for the [wfcommons](https://wfcommons.org) json workflow description format.
- many additional api functionality, typically as requested by users.
- scalability improvements both in terms of simulation time and simulation memory footprint.
- new and improved documentation.

**note**: wrench 2.0 requires [simgrid 3.31](https://simgrid.org)


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

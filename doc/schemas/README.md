<img src="../images/logo-vertical.png" width="100" />

# WRENCH Workflow Trace Schema (JSON)
Current Schema version: `1.0`


## Documentation
This documentation provides an overview of the latest version of the schema. Although this documentation tries to cover all aspects of it, we strongly recommend the use of a JSON schema validator before using your own traces.

- [x] `name`: Representative name for the trace name.
- [ ] `description`: A concise description of the trace. It should aid researchers to understand the purpose of the execution.
- [ ] `createdAt`: Schema creation date in the [ISO 8601](http://en.wikipedia.org/wiki/ISO_8601) format (e.g., `2016-11-27T15:19:28-08:00`).
- [x] `schemaVersion`: Version of the schema from an enumerate.
- [ ] `wms`: An `object` to describe the workflow management system (WMS) used to run the workflow.
- [x] `workflow`: An `object` to describe the workflow characteristics and performance metrics.
- [ ] `author`: An `object` to describe the information about the author who created the trace.


## WMS Property
The workflow management system property documents the WMS used to run the workflow. It is composed of the following sub-properties:

- [x] `name`: WMS name.
- [x] `version`: WMS version.
- [ ] `url`: URL for the main WMS website.


## Workflow Property
The workflow property is the core element of the trace file. It contains the workflow structure (jobs, dependencies, and files), as well as job characteristics and performance information. It is composed by the following sub-properties:

- [x] `makespan`: Workflow turnaround time in _seconds_.
- [x] `executedAt`: Workflow start timestamp in the [ISO 8601](http://en.wikipedia.org/wiki/ISO_8601) format (e.g., `2016-11-27T15:19:28-08:00`).
- [x] `jobs`: Sets of workflow jobs.
- [ ] `machines`: Sets of machines used for workflow jobs.

### Jobs Property
This property lists all jobs of the workflow describing their characteristics and performance metrics. Each job is described as an `object` property and is composed of 13 properties:

- [x] `name`: Job name.
- [x] `type`: Job type (whether it is a `compute`, `transfer`, or an `auxiliary` job).
- [ ] `arguments`: List of job arguments.
- [ ] `parents`: List of parent jobs (reference to other workflow jobs).
- [ ] `files`: Sets of input/output data.
- [x] `runtime`: Job runtime in _seconds_.
- [ ] `cores`: Number of cores required by the job.
- [ ] `avgCPU`: Average CPU utilization in %.
- [ ] `bytesRead`: Total bytes read in KB.
- [ ] `bytesWritten`: Total bytes written in KB.
- [ ] `memory`: Memory (resident set) size of the process in KB.
- [ ] `energy`: Total energy consumption in kWh.
- [ ] `avgPower`: Average power consumption in W.
- [ ] `priority`: Job priority.
- [ ] `machine`: Node name of machine on which job was run.

#### Files Property
The files property lists all files used throughout the workflow execution. Each `file` is listed as an `object` property, and is composed of the following properties:

- [x] `name`: A human-readable name for the file.
- [x] `size`: File size in KB.
- [x] `link`: Whether it is an `input` or `output` data.

#### Machines Property
The machines property lists all different machines that were used for workflow job execution. It is composed of the following properties:

- [ ] `system`: Machine system (linux, macos, windows).
- [ ] `architecture`: Machine architecture (e.g., x86_64).
- [x] `nodeName`: Machine node name.
- [ ] `release`: Machine release.
- [ ] `memory`: Total RAM memory in KB.
- [ ] `cpu`: An `object` to describe the machine's CPU information.

The **`cpu`** property is composed of a `count` (number of CPU cores), `speed` (CPU speed in MHz), and `vendor` (CPU vendor) properties.


## Author Property
The author property should contain the contact information about the person or team who created the trace. It is composed of the following properties:

- [x] `name`: Author name.
- [x] `email`: Author email.
- [ ] `institution`: Author institution.
- [ ] `country`: Author country (preferably country code, ISO ALPHA-2 code).

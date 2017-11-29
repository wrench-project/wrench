Overview                        {#mainpage}
============

@WRENCHUserDoc <div class="doc-type">User Documentation</div><div class="doc-link">Other: <a href="../developer/">Developer</a> - <a href="../internal/">Internal</a></div> @endWRENCHDoc
@WRENCHDeveloperDoc  <div class="doc-type">Developer Documentation</div><div class="doc-link">Other: <a href="../user/">User</a> - <a href="../internal/">Internal</a></div> @endWRENCHDoc
@WRENCHInternalDoc  <div class="doc-type">Internal Documentation</div><div class="doc-link">Other: <a href="../user/">User</a> -  <a href="../developer/">Developer</a></div> @endWRENCHDoc

[TOC]

![Workflow Management System Simulation Workbench](images/logo-vertical.png)

<br /><br />

[WRENCH](http://wrench-project.org) enables novel avenues for scientific workflow use, 
research, development, and education in the context of large-scale scientific 
computations and data analyses.
WRENCH capitalizes on recent and critical advances in the state of the art of distributed 
platform/application simulation. 
These advances make it possible to simulate the execution of large scale 
workflows in a way that is accurate (via validated simulation models), scalable 
(low ratio of simulation time to simulated time, ability to run large simulations 
on a single computer with low compute, memory, and energy footprints), and expressive (ability 
to describe arbitrary platform and application configurations, ability to prototype 
simulations quickly). More specifically, WRENCH is built on top of the open-source 
[SimGrid](http://simgrid.gforge.inria.fr) simulation framework.

Through the use of SimGrid, WRENCH provides the ability to: 

- Rapidly prototype implementations of Workflow Management System (WMS) components and underlying algorithms; 
- Quickly simulate arbitrary workflow and platform scenarios for a simulated WMS 
  implementation; and 
- Run extensive experimental campaigns results to conclusively compare workflow, platform, and
  WMS designs.


<br />

# Architecture #                        {#overview-architecture}

WRENCH is an _open-source library_ for developing simulators. It is neither a graphical 
interface nor a stand-alone simulator. WRENCH exposes several high-level simulation 
abstractions to provide the **building blocks** for developing custom simulators. 

WRENCH comprises four distinct modules, each designed as a simulation component:

- **Simulation Engine:** the simulation code that provides the necessary models to simulate arbitrarily interconnected hardware resources (compute, network, and storage).
- **Compute Services:** abstractions for the simulated infrastructure components that can execute workflow tasks (e.g., bare-metal servers, cloud platforms, batch-scheduled clusters, etc.).
- **Other Services:** abstraction for various services usable by a WMS.
- **Workflow Management System (WMS):** a top-level set of abstractions, the composition of which implements a simulated WMS.


![Overview of the WRENCH architecture.](images/wrench-architecture.png)


# Classes of Users #                       {#overview-users}

WRENCH is intended for three different classes of users:

- **WMS Users:** use WRENCH to guide their choices when executing their workflows using existing software insfrastructures.
- **WMS Developers:** use WRENCH to prototype and evaluate alternate software infrastructure designs for better supporting workflows.
- **WMS Researchers:** use WRENCH to investigate and evaluate novel efficient algorithms to be embedded within those software infrastructures that support workflows. 


## Levels of Documentation ##              {#overview-users-levels}

The WRENCH library provides three _incremental_ levels of documentation:

**User:** targets users who who want to use WRENCH for simulation the execution of scientific workflows in different simulation scenarios. _Users_ are NOT expected to develop new simulation abstractions or algorithms for WRENCH. Instead, they use available 
simulation components a high-level building blocks to build a simulator.
@WRENCHNotUserDoc ([See User Documentation](../user/index.html)) @endWRENCHDoc


**Developer:** targets _WMS developers_ and _WMS researchers_ that works on developing
novel algorithms, services, or computing environments. In addition to documentation 
for all simulation components provided at the _User_ level, documentation include
detailed documentation for all abstract classes for creating custom algorithms, 
services, or computing environments.
@WRENCHNotDeveloperDoc ([See Developer Documentation](../developer/index.html)) @endWRENCHDoc


**Internal:** targets those users who want to contribute code to WRENCH. The _internal_ level
provides, in addition to all levels above, a detailed documentation for all WRENCH classes
including binders to SimGrid.
@WRENCHNotInternalDoc ([See Internal Documentation](../internal/index.html)) @endWRENCHDoc


# Get in Touch #                        {#overview-contact}

The main channel to reach the WRENCH team is via the support email: 
[support@wrench-project.org](mailto:support@wrench-project.org?subject=WRENCH Support Contact: Your Topic).

**Bug Report / Feature Request:** our preferred channel to report a bug or request a feature is via  
WRENCH's [Github Issues Track](https://github.com/wrench-project/wrench/issues).

Overview                        {#mainpage}
============

![Workflow Management System Simulation Workbench](images/logo-vertical.png)

<br /><br />

[WRENCH](http://wrench-project.org) enables novel avenues for scientific workflow use, 
research, development, and education in the context of large-scale scientific 
computations and data analyses.
WRENCH capitalizes on recent and critical advances in terms of distributed 
platform/application simulation. 
These advances make it possible to simulate the execution of large scale 
workflows in a way that is both accurate (validated simulation models), scalable 
(low ratio of simulation time to simulated time, ability to run large simulations 
on a single computer with low hardware and energy costs), and expressive (ability 
to describe arbitrary platforms and applications, ability to quickly develop 
simulations). More specifically, WRENCH is built on top of the open-source 
[SIMGRID](http://simgrid.gforge.inria.fr) simulation framework.

Through the use of SIMGRID, WRENCH provides the ability to: 

- Rapidly prototype implementations of WMS components and underlying algorithms; 
- Quickly simulate arbitrary workflow and platform scenarios for a simulated WMS 
  implementation; and 
- Run extensive experimental campaigns results to conclusively compare workflow 
  designs and WMS designs.

[TOC]

<br />

## Architecture
________

WRENCH is composed by four modules designed as simulation components (see figure
below):

- **Simulation Engine:** actual simulation component – provides the necessary models to simulate individual hardware resources (compute, network, and storage resources)
- **Computing:** models for the target execution infrastructure (e.g., clouds, batch processing, etc.)
- **Services:** collection of services that can be attached to computing or used by the workflow management system
- **Workflow Management System:** top-level layer that provides a collection of managers and abstractions for composing a workflow management system


![Overview of the WRENCH architecture.](images/wrench-architecture.png)
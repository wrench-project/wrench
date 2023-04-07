.. _mainpage:

.. figure::  images/logo-horizontal.png
   :scale:   20%

`WRENCH <http://wrench-project.org>`__ is an open-source framework
designed to make it easy for users to develop accurate and scalable
simulators of distributed computing applications, systems, and
platforms. It has been used for research, development, and education.
WRENCH capitalizes on recent and critical advances in the state of the
art of simulation of distributed computing scenarios. Specifically,
WRENCH builds on top of the open-source
`SimGrid <https://simgrid.org>`__ simulation framework. SimGrid enables
the simulation of distributed computing scenarios in a way that is
accurate (via validated simulation models), scalable (low ratio of
simulation time to simulated time, ability to run large simulations on a
single computer with low compute, memory, and energy footprints), and
expressive (ability to simulate arbitrary platform, application, and
execution scenarios). WRENCH provides directly usable high-level
simulation abstractions, which all use SimGrid as a foundation, to make
it possible to implement simulators of complex scenarios with minimal
development effort.

In a nutshell, WRENCH makes it possible to:

- Develop in-simulation implementations of runtime systems that execute
  application workloads on distributed hardware platforms managed by
  various software services commonly known as Cyberinfrastructure (CI)
  services.
- Quickly, scalably, and accurately simulate arbitrary application and
  platform scenarios for these runtime system implementation.

.. _overview-architecture:

Architecture
============

WRENCH is an *open-source C++ library* for developing simulators. It is
neither a graphical interface nor a stand-alone simulator. WRENCH
exposes several high-level simulation abstractions to provide high-level
**building blocks** for developing custom simulators.

WRENCH comprises four distinct layers:

-  **Top-Level Simulation:** A top-level set of abstractions to
   instantiate a simulator that simulates the execution of a runtime
   system that executes some application workload on some distributed
   hardware platform whose resources are accessible via various
   services.
-  **Simulated Execution Controller:** An in-simulation implementation
   of a runtime system designed to execute some application workload.
-  **Simulated Core Services:** Abstractions for simulated
   cyberinfrastructure (CI) components that can be used by the runtime
   system to execute application workloads (compute services, storage
   services, network proximity services, data location services, etc.).
-  **Simulation Core:** All necessary simulation models and base
   abstractions (computing, communicating, storing), provided by
   `SimGrid <https://simgrid.org>`__.

.. figure::  images/wrench-architecture.png
   :scale:   30%
   :align:   center

.. _overview-users:

Three Classes of Users
======================

On can distinguish three kinds of WRENCH users:

-  **Runtime System Users** use WRENCH to simulate application workload
   executions using an already available, in-simulation implementation
   of a runtime system that uses Core Services to execution that
   workload.
-  **Runtime System Developers/Researchers** use WRENCH to prototype and
   evaluate runtime system designs and/or to investigate and evaluate
   novel algorithms to be implemented in a runtime system.
-  **Internal Developers** contribute to the WRENCH code, typically by
   implementing new Core Services.

.. _overview-users-levels:

Three Levels of API Documentation
---------------------------------

The WRENCH library provides three *incremental* levels of documentation,
each targeting an API level:

**User:** This level targets users who want to use WRENCH for simulating
the execution of application workloads using already implemented runtime
systems. *Users* are NOT expected to develop new simulation abstractions
or algorithms. Instead, they only use available simulation components as
high-level building blocks to quickly build simulators. These simulators
can involve as few as a 50-line of C++ code.

**Developer:** This level targets *runtime system developers and
researchers* who work on developing novel runtime system designs and
algorithms. In addition to documentation for all simulation components
provided at the *User* level, the *Developer* documentation includes
detailed documentation for interacting with simulated Core Services.
There are **two Developer APIs**. The most generic API is called the
*Action API*, and allows developers to describe and execution
application workloads that consist of arbitrary "actions". The
*Workflow API* is specifically designed for those developers that
implement workflow runtime systems (also known as Workflow Management
Systems, or WMSs), and as such is provides a Workflow abstraction that
these developers will find convenient. All details are provided in the
rest of the documentation.

**Internal:** This level targets those users who want to contribute code
to WRENCH. It provides, in addition to both levels above, detailed
documentation for all WRENCH classes including binders to SimGrid. This
is the API needed to, for instance, implement new Core Services.

.. _overview-contact:

Get in Touch
============

The main channel to reach the WRENCH team is via the support email:
`support@wrench-project.org <mailto:support@wrench-project.org?subject=WRENCH%20Support%20Contact:%20Your%20Topic>`__.

| **Bug Report / Feature Request:** our preferred channel to report a
  bug or request a feature is via
| WRENCH’s `Github Issues
  Track <https://github.com/wrench-project/wrench/issues>`__.


.. toctree::
   :hidden:
   :caption: Quickstart
   :maxdepth: 1

   installation.rst
   getting_started.rst

.. toctree::
   :hidden:
   :caption: Concepts
   :maxdepth: 1

   simulation_101.rst
   wrench_101.rst
   wrench_102.rst

.. toctree:: 
   :hidden:
   :caption: API Reference
   :maxdepth: 1

   api_user.rst
   api_developer.rst
   api_internal.rst
   rest_api.rst

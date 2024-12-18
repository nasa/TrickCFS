
# Introduction To TrickCFS

TrickCFS is a software package that provides the C structs, C++ classes and pertinant code required to synchronize a
core Flight Software (cFS) system with the Trick simulation executive. It also provides the capability to include
cFS-based application (App) data structures for generating the Trick interface code required to peek and poke cFS App data.

For a little background, cFS is a flight executive architecture used to build and run generic, configurable or specific
applications or "Apps" on multiple platforms. By providing a suite of core services in the core Flight Executive (cFE), 
flight software (FSW) developers can spend their valuable time configuring cFE for their target, developing 
flight-specific apps or developing operating system interface packages. The cFS architecture consists of four parts: 
the cFE, the Operating System Abstraction Layer (OSAL), and Platform Support Package (PSP), and cFS Applications. The 
OSAL and PSP are important concepts as it abstracts OS-specific operations from the application layer in both the cFE 
and any Apps. This allows one to build for multiple platforms using the same cFE and App code base by pointing to the 
appropriate OSAL and PSP packages. For more information on cFS, please visit https://github.com/nasa/cFS.

Trick is a simulation architecture that provides a build system, a software-based realtime and non-realtime simulation 
environment, a memory model to observe, record or change simulation data and a suite of tools to process simulation
data in useful ways. A Trick simulation can be used to model many things, including but not limited to, math models of
physical systems, firmware controllers, flight software algorithms and interfaces to external applications or devices.
For more information on Trick, please visit https://github.com/nasa/trick.

It is of great interest to both the cFS and Trick communities to integrate the two architectures together to provide a
simulation environment that combines models of physical and non-FSW systems with prototyped or fully developed FSW Apps
to simulate the closed-loop response of systems. Using standard Trick capabilities, perturbations, human interactions 
and off-nominal behaviors can be added to the simulated system that is very hard to generate and observe in a
non-simulation environment. Additionally, FSW events that normally take hours to test in a realtime environment can be
mimicked in a non-realtime simulation in much less time if the cFS and Trick components are sufficiently synchronized.
Rapid execution of this integrated environment allows users, developers and analysts to more efficiently vary the
conditions of the FSW and non-FSW components. For example, a user can use Trick's Monte Carlo simulation tools 
to perform risk analysis on the preliminary design of the physical components of a vehicle and/or the FSW algorithms 
controlling it. In the end, a Trick/CFS hybrid environment can only improve the user's options for design, development 
and testing of real-world systems.

# Documentation
- See docs/Release-Notes.txt to see changes since previous version
- See https://github.com/nasa/TrickCFS/releases/download/3.1.0-oss/TrickCFS.pdf for PDF version of the documentation
- To generate browser-based documentation execute `make doxy` from top directory with suggested minimal version Doxygen 
  v1.8.14

# Dependencies:
   Trick 19+ master branch from https://github.com/nasa/trick
   Known compatible CFS versions from https://github.com/nasa/cFS repo:
   - draco-rc4
   - equuleus-rc1
   - main branch as of hash 32eb0f824a7232345dbe2de325e9a1dff4eed332

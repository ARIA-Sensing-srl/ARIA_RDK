Introduction
ARIA RDK (Radar Development Kit) is a complete suite to evaluate ARIA Radar Devices and/or to develop custom application algorithms to radar data. ARIA RDK allows for the simultaneous data acquisition from one or more ARIA radar devices. Consequently, users may exploit ARIA RDK to develop different applications, ranging from single-device evaluation, to customer’s application development, and to advanced applications such as radar tomography.


Software Architecture
The system architecture is shown in Fig.1.

ARIA RDK includes four main components:
1. A Graphical User Interface (Windows and Linux) to create custom projects
2. An interface component to the Scientific Programming Language Octave to run custom algorithms on radar data
3. A Scheduler & Data I/O component to coordinate radar acquisitions and execute user scripts
4. A data workspace management and plotting component


Prerequisites 

ARIA RDK requires the following components to be compiled and installed in your system:
    • Qt6 (>= 6.7.2)
    • VTK: The Visualization Toolkit (https://vtk.org/)
    • JKQTPlotter: an extensive Qt5 & Qt6 Plotter Framework  
                                      (https://jkriege2.github.io/JKQtPlotter/index.html)
    • Eigen: C++ template library for linear algebra (>= 3.4.0) (https://eigen.tuxfamily.org/)
    • Octave (>=9.2.0): a scientific programming language environment
Please refer to respective build/install instruction.

ARIA-RDK is designed to be compiled with CMake®. Currently it has been compiled and tested under Clear Linux SO.

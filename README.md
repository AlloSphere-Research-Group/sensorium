# AlloOcean WIP

Developed by:

AlloSphere Research Group

University of California, Santa Barbara

# Installation
currently requires:
 * bash shell
 * git
 * cmake version 3.0 or higher

## How to compile / run
The src/ folder contains the initial main.cpp starter code.

On a bash shell you can run:

    ./configure.sh
    ./run.sh

This will configure and compile the project, and run the binary if compilation is successful.

Alternatively, you can open the CMakeLists.txt proeject in an IDE like VS Code, Visual Studio or Qt Creator and have the IDE manage the configuration and execution of cmake.

You can also generate other IDE projects through cmake.

## How to perform a distclean
If you need to delete the build,

    ./distclean.sh

should recursively clean all the build directories of the project including those of allolib and its submodules.

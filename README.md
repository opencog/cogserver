OpenCog CogServer
=================

[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver)

The OpenCog iCogserver is a Network and job server for the OpenCog
framework.

The main project site is at http://opencog.org

Overview
--------
The CogServer provides a network server and a job scheduler.


Building and Running
--------------------
For platform dependent instruction on dependencies and building the
code, as well as other options for setting up development environments,
more details are found on the [Building Opencog
wiki](http://wiki.opencog.org/wikihome/index.php/Building_OpenCog).

Prerequisites
-------------
To build and run the CogServer, the packages listed below are required.
With a few exceptions, most Linux distributions will provide these
packages. Users of Ubuntu may use the dependency installer from the
`/opencog/octool` repository.  Users of any version of Linux may
use the Dockerfile to quickly build a container in which OpenCog will
be built and run.

###### cogutil
> Common OpenCog C++ utilities
> http://github.com/opencog/cogutil
> It uses exactly the same build procedure as this package. Be sure
  to `sudo make install` at the end.

###### atomspace
> OpenCog Atomspace database and reasoning engine
> http://github.com/opencog/atomspace
> It uses exactly the same build procedure as this package. Be sure
  to `sudo make install` at the end.

Building OpenCog
----------------
Perform the following steps at the shell prompt:
```
    cd to project root dir
    mkdir build
    cd build
    cmake ..
    make
```
Libraries will be built into subdirectories within build, mirroring
the structure of the source directory root.


Unit tests
----------
To build and run the unit tests, from the `./build` directory enter
(after building opencog as above):
```
    make test
```

CMake notes
-----------
Some useful CMake's web sites/pages:

 - http://www.cmake.org (main page)
 - http://www.cmake.org/Wiki/CMake_Useful_Variables
 - http://www.cmake.org/Wiki/CMake_Useful_Variables/Get_Variables_From_CMake_Dashboards
 - http://www.cmake.org/Wiki/CMakeMacroAddCxxTest
 - http://www.cmake.org/Wiki/CMake_HowToFindInstalledSoftware


-Wno-deprecated is currently enabled by default to avoid a number of
warnings regarding hash_map being deprecated (because the alternative
is still experimental!)

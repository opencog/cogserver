OpenCog CogServer
=================

[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver)

The OpenCog Cogserver is a network and job server for the OpenCog
framework.

The main project site is at https://opencog.org

Overview
--------
The CogServer provides a network server and a job scheduler.
The network server provides a fast, efficient telnet interface,
giving access to scheme (guile) and python command-lines.
In particular, it is much faster (and more stable) than the native
guile read-evaluate-print-loop (REPL) network server.
Python does not even provide a networked REPL server.

Both can be used by multiple users at the same time, and provide
not only a convenient interface for ad-hoc issuing of commands,
but also provide a very good (and easy, and strong) way of doing
bulk data transfers. In particular, the transfer of multiple
megabytes of Atoms and (Truth)Values as UTF-8 (i.e. human-readable)
data is easy and efficient. In particular, it does not require
fiddling with complex binary formats or protocols or the use of
protocol libraries or API's.

The job scheduler is an experimental prototype for controlling
multiple threads and assigning thread processing priorities, in
an AtomSpace-aware fashion. It is in need of the caring love and
attention from an interested developer.

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

###### AtomSpace
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

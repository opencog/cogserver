OpenCog CogServer
=================

[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver)

The OpenCog Cogserver is a network and job server for the OpenCog
framework.

The main project site is at https://opencog.org

Overview
--------
The CogServer provides a network command-line console server and
a job scheduler.  The network console server provides a fast,
efficient telnet interface, giving access to scheme (guile) and
python command-lines. This itself is notable: by default, Python
does not allow multiple users to access it at the same time. As
to scheme/guile, there is an ice-9 REPL server, but the CogServer
is an order of magnitude faster, and infinitely more stable,
free of lockups, hangs and crashes.

Both Python and Scheme can be used by multiple users at the same
time, all obtaining access to the *same* AtomSpace.  Thus, the
Cogserver provides not only a convenient interface for ad-hoc
data processing, but also provides a very good (and easy, and strong)
way of doing bulk data transfers. In particular, the transfer of
multiple megabytes of Atoms and (Truth)Values as UTF-8
(i.e. human-readable) data is easy and efficient. In particular,
it does not require fiddling with complex binary formats or
protocols or the use of protocol libraries or API's. (We're looking
at you, HTTP, REST, ZeroMQ, ProtoBuff and friends. You are all
very sophisticated, yes, but are hard to use. And sometimes painfully
slow.)

The job scheduler (aka 'MindAgents' or just 'Agents') is an
experimental prototype for controlling multiple threads and assigning
thread processing priorities, in an AtomSpace-aware fashion. It is in
need of the caring love and attention from an interested developer.

For more info, please consult the
[CogServer wiki page](https://wiki.opencog.org/w/CogServer).

Building and Running
--------------------
The CogServer is build exactly the same way that all other OpenCog
components are built:
```
clone https://github.com/opencog/cogserver
cd cogserver
mkdir build
cd build
cmake ..
make -j
```
For additional information on dependencies and general hand-holding
with the build, see the [building Opencog
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
> OpenCog AtomSpace database and reasoning engine
> http://github.com/opencog/atomspace
> It uses exactly the same build procedure as this package. Be sure
  to `sudo make install` at the end.

Unit tests
----------
To build and run the unit tests, from the `./build` directory enter
(after building opencog as above):
```
    make test
```

Architecture
------------
See also these README's:

* [network/README](opencog/cogserver/network/README.md)
* [cogserver/README](opencog/cogserver/server/README.md)
* [builtin-module/README](opencog/cogserver/modules/commands/README.md)
* [cython/README](opencog/cython/README.md)
* [agents-module/README](opencog/cogserver/modules/agents/README.md)
* [events-module/README](opencog/cogserver/modules/events/README.md)

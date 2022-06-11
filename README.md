OpenCog CogServer
=================

opencog | singnet
------- | -------
[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver) | [![CircleCI](https://circleci.com/gh/singnet/cogserver.svg?style=svg)](https://circleci.com/gh/singnet/cogserver)

The OpenCog Cogserver is a network scheme/python command-line
and job server for the [OpenCog framework](https://opencog.org).

Overview
--------
The CogServer provides a network command-line console server and
a job scheduler.  The network console server provides a fast,
efficient telnet interface, giving access to Scheme (guile), Python
and JSON command-lines.  These can be used by multiple users at the
same time, all obtaining access to the *same* AtomSpace.  This
is useful in several different ways:

* General maintenance on long-running OpenCog or AtomSpace processes
  (e.g. robot control, large batch-job processing or long-running
  data-mining servers.) This includes running ad-hoc commands,
  monitoring status, and poking around and performing general
  maintenance on long-running servers.

* A network command line: by default, Python does not allow multiple
  users to access it at the same time.  As to scheme/guile, there is
  an ice-9 REPL server, but the CogServer is an order of magnitude
  faster, lower latency/higher throughput, and infinitely more stable;
  its free of lockups, hangs and crashes.

* A JSON-style interface, useful for creating javascript-powered
  visualizers and user interfaces.

* An easy and strong way of doing bulk data transfers. In particular,
  the transfer of millions of Atoms and (Truth)Values as UTF-8
  (i.e. human-readable) text is easy and efficient. It does not
  require fiddling with complex binary formats or protocols or the
  use of protocol libraries or API's. (We're looking at you, HTTP,
  REST, ZeroMQ, ProtoBuff and friends. You are all very sophisticated,
  yes, but are hard to use. And sometimes painfully slow.)

* Network-distributed processing. The [StorageNode
  API](https://wiki.opencog.org/w/StorageNode) provides a uniform data
  transfer API to local disk, 3rd-party databases and network. The
  CogServer implements this API, thus allowing multiple AtomSpaces
  distributed on the network to share data.

For more info, please consult the
[CogServer wiki page](https://wiki.opencog.org/w/CogServer).

Using
-----
There are three ways to start the cogserver: from a bash shell prompt
(as a stand-alone process), from the guile command line, or from the
python command line.

* From bash, just start the process:
  `$ build/opencog/cogserver/server/cogserver`

* From guile: `(use-modules (opencog cogserver)) (start-cogserver)`

* From python: `import opencog.cogserver` and then
   `??? start_cogserver() ???` (where's the documentation for this?)

Once started, one can obtain a shell by saying `rlwrap telnet localhost
17001`, and then `py`, `scm` or `json` to obtain python, scheme or json
shells.  This can be done as many times as desired; all shells share the
same AtomSpace, and the system is fully multi-threaded/thread-safe.
The status of all network connections is displayed by `stats`. For more
info, type `help py`, `help scm`, `help json` and `help stats`.

The `rlwrap` utility simply adds arrow-key support, so that up-arrow
provides a command history, and left-right arrow allows in-place editing.
Note that `telnet` does not provide any password protection!  It is
fully networked, so you can telnet from other hosts. The default port
number `17001` can be changed; see the documentation.

The socket protocol used is 'trivial'. Thus, besides `telnet`, one can
also use `netcat`, or access the socket directly, with ordinary socket
connect, open and read/write calls.

Building and Running
--------------------
The CogServer is built exactly the same way that all other OpenCog
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
To build and run the CogServer, you need to install the AtomSpace first.

###### AtomSpace
> OpenCog AtomSpace database
> http://github.com/opencog/atomspace
> It uses exactly the same build procedure as this package. Be sure
  to `sudo make install` at the end.

Unit tests
----------
To build and run the unit tests, just say
```
    make test
```
from the `./build` directory.

Architecture
------------
See also these README's:

* [network/README](opencog/network/README.md)
* [cogserver/README](opencog/cogserver/server/README.md)
* [builtin-module/README](opencog/cogserver/modules/commands/README.md)
* [cython/README](opencog/cython/README.md)
* [agents-module/README](opencog/cogserver/modules/agents/README.md)
* [events-module/README](opencog/cogserver/modules/events/README.md)

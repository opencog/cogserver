OpenCog CogServer
=================
[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver)

The OpenCog CogServer is a network scheme & python command-line
server for the [OpenCog AtomSpace](https://github.com/opencog/atomspace)
(hyper-)graph database. It allows network users to run arbitrary python
and scheme code on the server, and access the AtomSpace over the
network. The cogserver also provides a pseudo-JSON interface, as well
as a high-speed data transfer format that is used for building
network-distributed AtomSpaces. The CogServer is a part of the
[OpenCog project](https://opencog.org).

Overview
--------
The CogServer provides a network command-line console and a WebSocket
server.  The network console server provides a fast, efficient telnet
interface, giving access to Scheme (guile), Python and JSON command-lines.
These can be used by multiple users at the same time, all obtaining
access to the *same* AtomSpace. This is also shared by the WebSocket
interface, so that all users see the same data, irrespective of the
network connection.

This capability is useful in several different ways:

* **General maintenance** on long-running OpenCog or AtomSpace processes
  (e.g. robot control, large batch-job processing or long-running
  data-mining servers.) This includes running ad-hoc commands,
  monitoring status, and poking around and performing general
  maintenance on long-running servers.

* **Network command line.** Ordinary Python does not allow multiple
  users to access it at the same time. With the CogServer, multiple
  Python users can use it simultaneously. As to scheme/guile, whereas
  there is an ice-9 REPL server, the CogServer is an order of magnitude
  faster, lower latency/higher throughput, and infinitely more stable;
  its free of lockups, hangs and crashes. It's fast.

* **WebSocket API.** All interfaces are accessible through websockets.
  The only difference is that prompts are not sent. For example, the
  python API is available at `ws://localhost:18080/py`.  At this time,
  encryption is not supported, so `wss://` URL's will not work.
  See the [websocket example](./examples/websockets/demo.html) for more.

* **JSON-style interface.** This is useful for creating JavaScript-powered
  visualizers and user interfaces. Suitable for people who are more
  comfortable working with JSON.  This API is available at
  `ws://localhost:18080/json`.

* **Bulk data transfer.** The base "s-expression" encoding of Atoms and
  (Truth)Values is UTF-8 text string format. It's  human-readable, easy
  and efficient. It does not require fiddling with complex binary formats
  or protocols or the use of protocol libraries or API's. (We're looking
  at you, HTTP, REST, ZeroMQ, ProtoBuff and friends. You are all very
  sophisticated, yes, but are hard to use. And sometimes painfully slow.)

* **Network-distributed processing.** The [StorageNode
  API](https://wiki.opencog.org/w/StorageNode) provides a uniform data
  transfer API to local disk, 3rd-party databases and network. The
  CogServer implements this API, thus allowing multiple AtomSpaces
  distributed on the network to share data.

* **Proxy Agents.** The proxy infrastructure has been replaced by a
  more general, more flexible, more powerful and configurable proxying
  system. See the AtomSpace Storage git repo, in the
  [opencog/persist/proxy directory.](https://github.com/opencog/atomspace-storage/tree/master/opencog/persist/proxy)

* **Model Context Protocol.** An experimental MCP interface is being
  developed; this allows MCP-compatible LLM's to interact with the
  AtomSpace. There are two interfaces to it; one is plain-text, as would
  be the case for stdio MCP servers. The other is a standard HTTP
  connection.  See the [MCP README](./examples/mcp/README.md) for more
  info.

* The `stats` command provides a `top`-like command for viewing who is
  connected to the Cogserver, and what they are doing. Type `help stats`
  for more info.

For more info, please consult the
[CogServer wiki page](https://wiki.opencog.org/w/CogServer).

Version
-------
This is **version 3.2.0**. The code is stable, it's been used in
production settings for a decade.   There are no known bugs. There are
some planned features; see below.

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
  (At this time, there is no currently active python maintainer. Help
  wanted.)

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

WebSocket programmers will find it convenient to use their own favorite
tools to access the json API. A very simple example can be found in the
[WebSocket Example](examples/websockets/) directory.

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

The WebSockets server needs the OpenSSL devel environment to be
installed. Optional; if not installed, the cogserver will be built
without websockets support.

###### OpenSSL
> OpenSSL
> On Debian/Ubuntu, `sudo apt install libssl-dev`

The MCP Model Context Protocol server needs the NLohmann JSON devel
environment to be installed.  Optional; if not installed, the cogserver
will be built without MCP support.

###### NLohmann JSON
> JSON support library
> On Debian/Ubuntu, `sudo apt install nlohmann-json3-dev`


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

* [proxy/README](opencog/cogserver/proxy/README.md)
* [network/README](opencog/network/README.md)
* [cogserver/README](opencog/cogserver/server/README.md)
* [builtin-module/README](opencog/cogserver/modules/commands/README.md)
* [cython/README](opencog/cython/README.md)

TODO
----
There are two major open ToDo items for the CogServer.  These are:

* **Distributed computing.** How to build a distributed computing fabric
  for AtomSpace data? The read-thru and write-thru proxies provide basic
  building blocks for distributed computing: they can forward I/O
  traffic to other servers.
  See the [proxy/README](opencog/cogserver/proxy/README.md) for details.
  There are no automation tools for configuring these into a large complex
  network. There is a project for this in the
  [AtomSpace Agents](https://github.com/opencog/atomspace-agents) git
  repo. It is currently abandoned (due to lack of interest).

* **Security.** Right now, anyone who has network access can attach to
  the CogServer, and do anything. There are three different ways to do
  'anything':

  * The top-level CogServer shell allows arbitrary loadable modules to
    be loaded.
  * The python and scheme shells allow the execution of arbitrary python
    and scheme code, including system calls. This includes file writes.
  * The sexpr and json shells are "controlled", in that they do NOT
    allow access to system calls.  However, they do allow arbitrary
    changes to the made to the AtomSpace, including insertion and
    deletion of Atoms. The `GroundedPredicateNode` allows for the
    arbitrary execution of arbitrary scheme and python code.

It's not clear how to introduce security into this model, other than to
only open network connections to trusted, authorized users. Presumably,
there are several off-the-shelf solutions for controlling network
access, but no one has picked a good one. Obviously, exporting the
CogServer socket via SSH is a good way of controlling access to who
can use it. But SSH proxying is very low-level, and not admin-friendly.
Nor particularly user-friendly, either: the user can't just login into
some website and ask for access.

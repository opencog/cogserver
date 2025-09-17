OpenCog CogServer
=================
[![CircleCI](https://circleci.com/gh/opencog/cogserver.svg?style=svg)](https://circleci.com/gh/opencog/cogserver)

The OpenCog CogServer is a network scheme & python command-line
server for the [OpenCog AtomSpace](https://github.com/opencog/atomspace)
(hyper-)graph database. It allows network users to run arbitrary python
and scheme code on the server, and access the AtomSpace over the
network. It also provides a JSON interface, a high-performance
s-expression bulk data transfer format, and an LLM MCP format.
The s-expression interface is used for building network-distributed
AtomSpaces. The CogServer is a part of the
[OpenCog project](https://opencog.org).

Overview
--------
The CogServer provides a network command-line console, a WebSocket
server and an HTTP server.  The network console server provides a
fast, efficient telnet interface, giving access to Scheme (guile),
Python, JSON, and s-expression command-lines.  These can be used by
multiple users at the same time, all obtaining access to the *same*
AtomSpace. This is also shared by the HTTP and WebSocket interfaces,
so that all users see the same data, irrespective of the network
connection.

This capability is useful in several different ways:

* **General maintenance** on long-running OpenCog or AtomSpace processes
  (e.g. robot control, large batch-job processing or long-running
  data-mining servers.) This includes running ad-hoc commands,
  monitoring status, and poking around and performing general
  maintenance on long-running servers.

* **Multi-user Python REPL.** Ordinary Python does not allow
  multiple users to access it at the same time.  The CogServer
  provides a python network shell, which can be used by multiple
  users at the same time.  All python state is visible to all
  python users. All changes to the AtomSpace become immediately
  available to Python, Scheme, JSON, s-expression and MCP users.

* **Fast Scheme REPL.** The CogServer includes a scheme (guile)
  interface.  It is an order of magnitude faster than the `ice-9`
  REPL server, with much lower latency and higher throughput.
  It is also stable; its free of lockups, hangs and crashes. It's fast.

* **Bulk data transfer.** The CogServer includes a raw s-expression
  interface. This is another order of magnitude faster than either
  python, scheme or JSON, and allows bulk data transfer.  It is
  also several orders of magnitude faster than conventional ZeroMQ,
  HTTP, RPC, RPC/JSON or Protobuff protocols.

* **WebSocket API.** All interfaces are accessible through websockets.
  The only difference is that the REPL prompts are not sent. For example,
  the python API is available at `ws://localhost:18080/py`.
  See the [websocket example](./examples/websockets/demo.html) for more.

* **JSON-style interface.** This is useful for creating JavaScript-powered
  visualizers and user interfaces. Suitable for people who are more
  comfortable working with JSON.  This API is available at
  `ws://localhost:18080/json`.

* **Network-distributed processing.** The [StorageNode
  API](https://wiki.opencog.org/w/StorageNode) provides a uniform data
  transfer API to local disk, 3rd-party databases and network. The
  CogServer implements this API, thus allowing multiple AtomSpaces
  distributed on the network to share data.

* **Proxy Agents.** Proxies allow mirroring, load-balancing and
  caching.  See the AtomSpace Storage git repo, in the
  [opencog/persist/proxy directory](https://github.com/opencog/atomspace-storage/tree/master/opencog/persist/proxy)
  in the `atomspace-storage` git repo.

* **Model Context Protocol.** An MCP interface has been developed;
  this allows MCP-compatible LLM's to interact with the AtomSpace.
  You can examine and change AtomSpace contents merely by asking the
  LLM to do it for you! No more coding in Atomese! Just talk to it!
  There are three interfaces: standard HTTP, websockets and raw
  JSON/RPC over a TCP/IP socket.
  See the [MCP README](./examples/mcp/README.md) for more info.

* **Demo visualizer.** This is a simple html/js visualizer for the
  AtomSpace contents, accessible with standard web browsers. Just
  start the CogServer, open http://localhost:18080/ and go.

For more info, please consult the
[CogServer wiki page](https://wiki.opencog.org/w/CogServer).

Version
-------
This is **version 3.3.0**. The code is stable, it's been used in
production settings for over a decade.   There are no known bugs.

Using
-----
There are three ways to start the cogserver: from a bash shell prompt
(as a stand-alone process), from the guile command line, or from the
python command line.

* From bash, after installing, just start the process:
  `$ cogserver`
  Add the `-h` flag to get a list of configurable settings.

* From guile: `(use-modules (opencog cogserver)) (start-cogserver)`
  Documentation is available at the guile REPL with the
  `,describe start-cogserver` command.

* From python:
```
    from opencog.atomspace import AtomSpace
    from opencog.cogserver import *
    my_atomspace = AtomSpace()
    start_cogserver(atomspace=my_atomspace)
```
See the [python example](./examples/python/start_cogserver.py) for
more details.

There are several ways to interact with a running cogserver. One way
is through HTTP or websockets: point your web browser at
`http://localhost:18080` and go.

To use an LLM to view and manipulate the AtomSpace, just tell the LLM
where to find it. For Anthropic Claude, this  would be:
```
claude mcp add atomese -t http http://localhost:18080/mcp
```

The command-line telnet shell is accessed by saying
`rlwrap telnet localhost 17001`. Type `help` for a list of available
commands. These include the `py`, `scm`, `json` and `sexpr` shells
(for python, scheme, json and s-expressions.) A one-shot python
evaluator is available at `py-eval`: it will run a blob of python
code and return immediately, without providing a shell.

The system is multi-user: all shells share the same AtomSpace.
The status of all network connections is displayed by `stats` and
`top`. For more info, type `help py`, `help scm`, `help json` and
`help stats`.

By using `telnet` with `rlwrap`, one gets arrow-key support, so that
up-arrow provides a command history, and left-right arrow allows
in-place editing.

There is no authentication or password protection! If you need that,
you need to download, install and configure an authentication server
that can wrap the CogServer.

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
sudo make install
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

###### ASIO
> The ASIO asynchronous IO system.
> On Debian/Ubuntu, `sudo apt install libasio-dev`

###### OpenSSL
> Optional: OpenSSL
> On Debian/Ubuntu, `sudo apt install libssl-dev`
> If you don't install SSL, you won;t get websockets.

###### JSON
> Optional: JSON support library
> On Debian/Ubuntu, `sudo apt install libjsoncpp-dev`
> If you don't install JSON, you won't get JSON.


Unit tests
----------
To build and run the unit tests, just say
```
    make -j check
```
from the `./build` directory.

Architecture
------------
The system architecture is described in these README's:

* [network/README](opencog/network/README.md)
* [cogserver/README](opencog/cogserver/server/README.md)
* [builtin-module/README](opencog/cogserver/modules/commands/README.md)
* [cython/README](opencog/cython/README.md)

TODO
----
* Add `wss://` and `https://` encryption.
* Add OAuth authentication. This should be provided by some external
  server wrapper, but how?
* What about authentication and encryption for the raw telnet/netcat
  interfaces?

----

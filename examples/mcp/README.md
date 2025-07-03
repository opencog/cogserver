MCP For the CogServer
---------------------
An experimental interface supporting the Model Context Protocol (MCP)
has been created. In it's current form, it allows LLM's to work with
AtomSpace contents in the CogServer.

The eventual goal is to be able to chat with the AtomSpace to accomplish
complex, sophisticated hypergraph manipulations, working entirely in
[Atomese](https://wiki.opencog.org/w/Atomese). For now, only the basic
AtomSpace API's have been implemented; this implementation can be found
in the [atomspace-storage JSON
directory](https://github.com/opencog/atomspace-storage/tree/master/opencog/persist/json).

Some example mini-prompts include:
* Please ask the atomese MCP server what version it is.
* ask the atomese server what all the direct subtypes of type 'Node' are
* run that query again, but set subclass to true
* ask if it has a node of type 'Concept' that is named 'foo'
* Are there any atoms of type Node?
* Can you run that query again, setting subclass to true?
* Please ask the server if it has (ListLink (Concept "foo"))
* are there any Links that contain a ConceptNode named 'foo'
* please make an atom called bar of type ConceptNode
* The contents of the atomese server can change any time. If I ask to do it
  again, this is because things may have changed.
* please get the incoming set of the ConceptNode foo
* Please create (Concept "bar") and (Concept "foo")
* Please create a node called (Concept "fimble")
* Please attach a value to this atom. The value should be located at
  the key (Predicate "fovs") and it should be a FloatValue holding the
  vector 1 2 3 0.4 0.5 -0.6 0.777 88 999

The eventual goal of attaching MCP to the AtomSpace is not to access
Atoms one or two at a time, but to manipulate millions of them, using
Atomese sensorimotor interfaces. These are under development, and are
"pre-alpha" (cough cough. Version 0.0.1 to be precise.) For now, the
above works.

Note that the above requests work just fine, even without any additional
prompting about the AtomSpace. The [CLAUDE.md](CLAUDE.md) file contains
a large, detailed prompt explaining the AtomSpace to Claude (or any
other LLM with MCP support).

### TODO
* Teach Claude how to run AtomSpace queries -- i.e. how to write
  `QueryLink` and then run it.
* Teach Claude how to create a data processing pipeline: how to write
  Atomese needed to compute cosine similarity or mutual information
  for some collection of `EdgeLink`s, i.e. try to get it to reinvent
  the old atomspace-matrix code, but this time in pure Atomese.
* Move the CLAUDE.md file over to an MCP "resource". That is, I think
  this is the intended way of providing MCP documentatio. I guess.
  I'm not sure.

The MCP Servers
---------------
There are two ways of using MCP with the CogServer:
* Over an HTTP connection, where the CogServer acts as a web server,
* Over a raw TCP/IP socket, which reads and responds to MCP JSON.

The first form provides a conventional HTTP interface, as documented at
[modelcontextprotocol.io](https://modelcontextprotocol.io/). The second
form exists for custom applications that wrap MCP in other ways, and
need a raw JSON interface. Currently, the major application is to work
around the open bug
[Claude Code #1536](https://github.com/anthropics/claude-code/issues/1536).
The work-around is given further below, and uses a pair of proxies to
copy MCP JSON on stdio to the raw CogServer socket.

The regular interface is located at port 18080, at the URL `/mcp`. For
example, Claude can access this after configuring
```
claude mcp list
claude mcp add atomese -t http http://localhost:18080/mcp
```
The raw interface is at port 18888; an example of how to use it is given
below.


MCP Utility Tools
-----------------
Due to the open bug
[Claude Code #1536](https://github.com/anthropics/claude-code/issues/1536),
some proxy tools are provided in this directory.

### Socket Proxies
Due to networking issues or technical issues or bugs, it can be the case
that an LLM cannot contact the CogServer directly. The pair of proxy
servers `stdio_to_unix_proxy.py` and `unix_to_tcp_proxy.py` can be used
to overcome/bypass these issues.

For example, if using Claude Code, the proxy can be configured as
```
claude mcp list
clause mcp add atomese /where/ever/stdio_to_unix_proxy.py
```
Then, in a distinct shell, run
```
./unix_to_tcp_proxy.py --remote-host example.com --remote-port 18888 --verbose
```
and then make sure that the CogServer is running on that host. The
default CogServer MCP port is 18888.

### Demo MCP checker
The code in `mcp-checker.cc` provides a simple Model Context protocol
(MCP) test client that can be used to verify that an MCP network server
is available, and is responding to commands.  It will list the tools
and resources provided by the MCP server. By default, it connects to
`localhost:18888`, which is where the raw CogServer MCP port is located.
Use the `--host` and `--port` flags to specify a different host and
port.

The binary is located at [build/examples/mcp](../../build/examples/mcp).

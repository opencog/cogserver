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
* run that query again, but set subclass to true.
* ask if it has a node of type 'Concept' that is named 'foo'
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

The above works without having to explain anything to the LLM. The MCP
interfaces describe the AtomSpace and how it works to the LLM.

The eventual goal of attaching MCP to the AtomSpace is not to access
Atoms one or two at a time, but to manipulate millions of them, using
Atomese sensorimotor interfaces. These are under development, and are
"pre-alpha" (cough cough. Version 0.0.2 to be precise.) For now, the
above works.

The MCP Servers
---------------
There are three ways of using MCP with the CogServer:
* Over a raw TCP/IP socket.
* Over a websockets connection.
* Over an HTTP connection.

All three are effectively "the same thing", differing only in how the
MCP JSON responses are wrapped. The MCP JSON protocol is documented at
[modelcontextprotocol.io](https://modelcontextprotocol.io/).

The raw TCP/IP interface is at port 18888.  The HTTP and websocket
interfaces are located at port 18080, at the URL `/mcp`.

The raw interface should be the most performant. It can be configured as
```
claude mcp list
claude mcp add atomese nc localhost 18888
claude mcp list
```
The second `claude mcp list` confirms that the interface was found and
that Claude was able to connect. Some versions of Claude are buggy, and
don't connect! Some vesions of Claude are fine!

The http transport is also possible. Some versions of Claude are buggy,
and will fail to connect to the http transport. You can configure as
follows; just be sure to double-check that `claude mcp list` shows that
the tool is connected!
```
claude mcp list
claude mcp add atomese -t http http://localhost:18080/mcp
claude mcp list
```


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

Ideally, start the CogServer first.  Then, in a distinct shell, run
```
./unix_to_tcp_proxy.py --verbose
```
The above connects to the CogServer on localhost. Specify a remote
CogServer like so:
```
./unix_to_tcp_proxy.py --remote-host example.com --remote-port 18888 --verbose
```
The default CogServer raw MCP port is 18888.

Next, tell the LLM how to connect.  If using Claude Code, the proxy
can be configured as
```
claude mcp list
clause mcp add atomese /where/ever/stdio_to_unix_proxy.py
claude mcp list
```

The second `claude mcp list` will connect and verify that the connection
is working.

### Demo MCP checker
The code in `mcp-checker.cc` provides a simple Model Context protocol
(MCP) test client that can be used to verify that an MCP network server
is available, and is responding to commands.  It will list the tools
and resources provided by the MCP server. By default, it connects to
`localhost:18888`, which is where the raw CogServer MCP port is located.
Use the `--host` and `--port` flags to specify a different host and
port.

The binary is located at [build/examples/mcp](../../build/examples/mcp).

TODO
----
Future things.
* Teach Claude how to run AtomSpace queries -- i.e. how to write
  `QueryLink` and then run it.
* Teach Claude how to create a data processing pipeline: how to write
  Atomese needed to compute cosine similarity or mutual information
  for some collection of `EdgeLink`s, i.e. try to get it to reinvent
  the old atomspace-matrix code, but this time in pure Atomese.

-----

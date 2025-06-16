MCP For the Cogserver
---------------------
An experimental interface supporting the Model Context Protocol (MCP)
has been created. In it's current form, it allows LLM's to work with
AtomSpace contents in the CogServer.

The eventual goal is to be able to chat with the AtomSpace to accomplish
complex, sophisticated hypergraph manipulations, working entirely in
Atomese. For now, only the basic AtomSpace API's have been implemented;
this implmentation can be found in the [atomspace-storage JSON
directory](https://github.com/opencog/atomspace-storage/tree/master/opencog/persist/json).

Some example queries include:
* Please ask the cogserv MCP server what version it is.
* ask the cogserv server what all the direct subtypes of type 'Node' are
* run that query again, but set subclass to true
* ask if it has a node of type 'Concept' that is named 'foo'
* Are there any atoms of type Node?
* Can you run that query again, setting subclass to true?
* Please ask the server if it has (ListLink (Concept "foo"))
* are there any Links that contain a ConceptNode named 'foo'
* please make an atom called bar of type ConceptNode
* The contents of the cogserv can change any time. If I ask to do it
  again, this is because things may have changed.
* please get the incoming set of the ConceptNode foo
* Please create (Concept "bar") and (Concept "foo")
* Please create a node called (Concept "fimble")
* Please attach a value to this atom. The value should be located at
  the key (Predicate "fovs") and it should be a FloatValue holding the
  vector 1 2 3 0.4 0.5 -0.6 0.777 88 999

Due to an open bug in Claude Code, some proxy tools are provided in this
directory. Read about them below.

The eventaul goal of attaching MCP to the AtomSpace is not to access
Atoms one or two at a time, but to manipulate millions of them, using
Atomese sensorimotor interfaces. These are under development, and are
"pre-alpha" (cough cough. Version 0.0.1 to be precise.) For now, the
above works.

MCP Utility Tools
-----------------

### Socket Proxies
Due to technical issues, it can be the case that an LLM cannot contact the
CogServer directly. The pair of proxy servers `stdio_to_unix_proxy.py`
and `unix_to_tcp_proxy.py` can be used to overcome/bypass these issues.

For example, if using Claude Code, the proxy can be configured as
```
claude mcp list
clause mcp add cogserv /where/ever/stdio_to_unix_proxy.py
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
`localhost:18888`, which is where the CogServer MCP port is located.
Use the `--host` and `--port` flags to specify a different host and
port.

The binary is located at [build/examples/mcp](../../build/examples/mcp).

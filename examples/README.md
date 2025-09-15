Examples
--------
Every running CogServer provides a web page showing the current status
of the server. This can be viewed by opening port 18080 in a web browser.
If the CogServer is running on the local machine, then it can be accessed
as [http://localhost:18080/stats](http://localhost:18080/stats). The status
page includes a list of the currently loaded modules, and a `top`-like display,
showing all connected clients, and the network status for each client.

The [websockets](./websockets) subdirectory contains a web page that
shows how to interact with a running cogserver.  Just load the
[demo.html](./websockets/demo.html) page in a browser,
[***like this***](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/websockets/demo.html),
and go.
All the actual network i/o is done with the
[script.js](./websockets/script.js) javascript file. The
[json-test.html](./websockets/json-test.html) page shows network
traffic for a basic JSON session.
[***Run it here***](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/websockets/json-test.html).

The [visualizer](./visualizer) subdirectory contains a web page that
provides some basic info about the contents of the AtomSpace. This
includes the total counts for various atom types, and some basic
browsing ability.
[***Run it here***](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/visualizer/index.html).

The [mcp](./mcp) subdirectory contains a Model Context Protocol (MCP)
several prompts, originally written for Claude, that explain what the
AtomSpace is, and how to use it. Claude, or any other LLM capable of
chatting the MCP protocol can connect to the CogServer at port 18888
and work directly with the Atoms in the AtomSpace.

The mcp subdirectory also includes some debug and checking tools.
One of these verifies that the CogServer is responding correctly.
Two more are network proxies that can be used to escape network
sandboxes. These are useful when running Claude over TOR or a VPN.

The [python](./python) subdirectory demonstrates how to start the
cogserver from a python shell, so that the AtomSpace used by the python
shell is the same one as that used by the cogserver.

The demo for running the cogserver from scheme is almost trivial.
Just do this:
```
(use-modules (opencog) (opencog cogserver))
(start-cogserver)
```
That's it. Scheme documentation available from the guile REPL, by
saying `,describe start-cogserver`.

The [module](./module) subdirectory provides a template for creating
a custom module. Most developers should NOT be interested in this
example! If you want to create a new shell, similar to the python,
scheme or json shells, then look at the code for those, located at
`opencog/cogserver/shell/` and emulate what you find there.

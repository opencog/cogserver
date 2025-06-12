
Examples
--------

Every running CogServer provides a web page showing the current status
of the server. This can be viewed by opening port 18080 in a web browser.
If the CogServer is running on the local machine, then it can be accessed
as [http://localhost:18080/](http://localhost:18080/). The status
includes a list of the currently loaded modules, and a `top`-like display,
showing all connected clients, and the network status for each client.

The [websockets](./websockets) subdirectory contains a web page that
shows how to interact with a running cogserver.  Just load the
[demo.html](./websockets/demo.html) page in a browser, and go.
All the actual network i/o is done with the
[script.js](./websockets/script.js) javascript file.

The [module](./module) subdirectory provides a template for creating
a custom module. Most developers should NOT be interested in this
example! If you want to create a new shell, similar to the python,
scheme or json shells, then look at the code for those, located at
`opencog/cogserver/shell/` and emulate what you find there.

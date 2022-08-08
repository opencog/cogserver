
CogServer
=========
The CogServer provides a network shell server and command dispatcher,
allowing multiple users to use the OpenCog system, including the
scheme (guile) REPL shell, and the Python REPL shell. Particularly
notable is that multiple users can use the same server at the same time,
using python and scheme together, as desired, always obtaining access
to the same AtomSpace.  This is immensely useful for monitoring large
batch jobs, as the job can be launched on one terminal, and monitored
from another. It is also tremendously useful for interfacing to ROS
(The Robot Operating System) as ROS is easily accessed in Python, and
so the CogServer provides a natural bridge between ROS and compute
processes running in OpenCog.

Using
-----
To access the shell server, build everything, install, and then run
`cogserver` at the bash prompt (alternately, run it from the build
directory at `build/opencog/cogserver/server/cogserver`.  Then, from
another terminal, run `rlwrap telnet localhost 17001`; this will
connect to the shell.  Type `help` at the shell prompt to get a
listing of the commands. Type `scm` to get to the guile scheme shell;
type `py` to get the python shell.  (The `rlwrap` function provides
an editable command-line history (i.e. enables the arrow-keys.))

This is a generic network server, and so `netcat localhost 17001`
also works. Long or complex sequences of commands may be placed in a
file, and loaded by saying `cat somefile | netcat localhost 17001`.

A WebSocket server is provided at the default port of 18080. This
can be accessed using standard WebSocket API's. Examples:
* A json server at `ws://localhost:18080/json`
* A scheme server at `ws://localhost:18080/scm`
* A python server at `ws://localhost:18080/py`
At this time, encryption is not supported; the `wss://` URL's won't
work.

Server stats can be viewed as an ordinary web page, at
`http://localhost:18080/`.

A non-default network port can set with the `-p` option, and an
alternate websocket port with the `-w` option on the cogserver.

From guile
----------
The cogserver can be started from guile, as follows:
```
$ guile
(use-modules (opencog))
(use-modules (opencog cogserver))
(start-cogserver)
```
Non default ports can be specified:
```
(start-cogserver #port 17002 #:web 8080)
```
Enter `,descri start-cogserver` for more options.

Loadable Modules
----------------
The loadable module subsystem is deprecated; interested users should
consider writing Scheme (guile) or Python modules instead.

The CogServer can be extended with task-specific dynamically loadable
modules. These can be loaded at run-time, with the `loadmodule`
CogServer shell command.

Modules can be automatically loaded on server startup by specifying
them in a `cogserver.conf` file, and adding the module name to the
list therein.  By default, the `/etc/cogserver.conf` file is used;
or you can start to cogserver with `cogserver -c myconfig.conf`.

Loadable modules will typically need to expose new commands. There are
four ways of doing this: by using `DECLARE_CMD_REQUEST` (to define a
small handful of commands), by using the `GenericShell` class (to build
a custom shell command evaluator), or by using scheme or python.

Additional interfaces can be created by specializing
`class GenericShell`.  The GenericShell bypasses the CogServers's
command processor, and passes input data over to the overloaded
generic `eval()` method, which is then free to interpret the input
in any way desired.

TODO
----
Get rid of BaseServer. It's not needed any more.

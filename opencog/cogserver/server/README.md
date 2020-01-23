
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

Long-term strategy
------------------
Long term, it would probably be best to eliminate the module and request
subsystems; these do not offer any abilities that cannot be performed
more easily or elegantly by using either Scheme or Python modules.
This removal cannot be currently done, as there are too many dependent
users (most notably, the ECAN Attention Allocation subsystem).

After the Request and Module subsystems are removed, what's left is the
network server. This part is worth keeping, mostly because there does
not seems to be any existing alternative out there.  For example, Python
does not offer a networked shell. Guile does, but the current guile
implementation is painfully slow, crashy, and subject to hangs. The
OpenCog wiki does describe an attempt to emulate the network server with
a clever use of netcat.  That ... almost works, but not quite. Thus,
there is a real need for the network server.

The MindAgents idea is interesting but immature. The idea here is that
an OpenCog may want some sort of sophisticated thread scheduling (either
using kernel threads, or using shared cooperative threads).
Unfortunately, it is not clear who the users of this facility would be,
or what they would want it to do. Thus, its not clear how to design
this properly.

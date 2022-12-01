OBOSLETE
========
The code below has been obsoleted by the base AtomSpace ProxyNode
infrastructure. The code here worked. Just that it is not really
the best design ... it was hard to configure. Not flexible. The
ProxyNode infrastrcuture is superior. We keep this here as a curio.

Proxy Agents
------------
The code here implements a form of AtomSpace proxying; that is, of
sending and receiving Atoms over the network, and passing them onwards
to other StorageNodes.

In the standard mode, when a network connection is made to the
CogServer, the user at the far end of the network connection is working
with the AtomSpace that this CogServer holds. It is reasonable to want
to have the CogServer attached to storage (say, disk storage), so that
when an Atom is received, it is also written to disk.  This general
idea is called "proxying", and "proxy agents" are responsible to doing
whatever needs to be done, when a read or write request for an Atom is
received by the CogServer. The proxy agent passes on those requests to
other StorageNodes.

The diagram below shows a typical usage scenario. In this example,
the Link Grammar parser is reading from the local AtomSpace to which
it is attached. That AtomSpace uses a CogStorageNode to get access
to the "actual data", residing on the CogServer. However, the CogServer
itself doesn't "have the data"; its actually on disk (in the
RocksStorageNode) Thus, any read requests made by the the LG parser
have to be proxied by the CogServer to the disk storage (using a
read-thru proxy.)
```
                                            +----------------+
                                            |  Link Grammar  |
                                            |    parser      |
                                            +----------------+
                                            |   AtomSpace    |
    +-------------+                         +----------------+
    |             |                         |                |
    |  CogServer  | <<==== Internet ====>>  | CogStorageNode |
    |             |                         |                |
    +-------------+                         +----------------+
    |  AtomSpace  |
    +-------------+
    |    Rocks    |
    | StorageNode |
    +-------------+
    |   RocksDB   |
    +-------------+
    | disk drive  |
    +-------------+
```

Status
------
**Version 1.0.0**. Both the read-thru and the write-thru proxies work.
There are no known bugs.

They have been used in a "demanding" environment: multi-day data
processing runs, transferring many gigabytes of data. Everything looks
good. They've not yet been used in "production" runs (runs that burn
through terabytes of data).

HOWTO & Demo
-------------
Follow these steps to test by hand. The demo requires two machines: the
"remote" server and the  "local" client.  They can both be localhost, as
long as you keep tabs on which is which. In the below, the "remote" machine
has the name `example.com`; replace this by the actual hostname or
dotted IPv4 address.

First, log into `example.com` and start a cogserver:
```
$ guile
guile> (use-modules (opencog) (opencog persist) (opencog persist-rocks))
guile> (use-modules (opencog cogserver))
guile> (define rsn (RocksStorageNode "rocks:///tmp/foo.rdb"))
guile> (cog-open rsn)
guile> (start-cogserver)
```
Then, on the local machine machine, establish a connection to the
server:
```
$ guile
guile> (use-modules (opencog) (opencog persist) (opencog persist-cog))
guile> (define csn (CogStorageNode "cog://example.com?w-thru"))
guile> (cog-open csn)
guile> (store-atom (Concept "foo" (stv 0.123 0.456)))
guile> (cog-close csn)
guile> ^D  ; exit the guile shell
```
Replace `example.com` by the actual hostname, or just use `localhost`
if both local and remote are the same machine. Dotted addresses, such
as `192.168.0.42` and `127.0.0.1` also work.

The above should have written a single Atom to the Rocks DB on the remote
machine.  Verify this by exiting the guile shell on the remote machine, and
then restarting it, loading the AtomSpace, and taking a look at it:
```
$ guile
guile> (use-modules (opencog) (opencog persist) (opencog persist-rocks))
guile> (define rsn (RocksStorageNode "rocks:///tmp/foo.rdb"))
guile> (cog-open rsn)
guile> (load-atomspace)
guile> (cog-prt-atomspace)
```
Observe that this AtomSpace now contains an Atom called `(Concept "foo")`
and that it has the correct TruthValue `(stv 0.123 0.456)`. That's it.
Notice that the `(store-atom stuff)` in the local AtomSpace resulted in
a store on the remote machine.

The above only used the write through proxy.  Both can be used together
by specifying the URL
```
   cog://example.com?r-thru&w-thru
```

The read-thru proxy can be used all by itself:
```
   cog://example.com?r-thru
```

See below for details.

TODO
----
***Stop the presses!***. OK everything described in the later sections
works, but now we've got a much much better idea, and it neeeds to be
implemented. It will make everything here "obsolete" (because it will
be built-in).

Create a `ProxyStorageNode`. It's a `StorageNode`, like all the others,
providing the same API. But it implements all the decision-making about
"what to do" when some particular read/write request is made, inside
itself. So, for example, there's a `ReadThruProxyStorageNode` and a
`WriteThruProxyStorageNode`.

Before the user starts the cogserver, the user will create one or both
of these nodes, and open them.

When starting the cogserver, the user will tell the cogserver to use
one of these proxies. If none is given, it will use the normal mode.
(perhaps the normal mode can be a TransparantProxyStorageNode that
"does nothing" but access the AtomSpace directly.)

That's it. Proxying is now "built in" and "always on". No need to
fiddle with loadmodule crap. Proxying now "moves back" to the
AtomSpace, where all kinds of wild and crazy things can be built,
with zero dependence on the cogserver.  Those proxy nodes can then
also be used in other ways, for other reasons!

The proxy nodes can be used to build processing pipelines....

Configuration is delegated to the ProxyNode.  Perhaps it needs to
be a ProxyLink, for parameters?

That's it! Simple, easy, solves a mess of the issues raise below.


Design Choices
--------------
Ruminations about design choices. During development, several design
choices were considered.

* Create a wrapper around the AtomSpace, so that everything going
  through the AtomSpace goes through the wrapper first. The CogServer
  is then configured to use this wrapper. The problem with this
  particular idea is that Atoms can also be accessed directly *e.g.* to
  get Values or IncomingSets, and so Atoms would need to be wrapped as
  well. This appears to become a difficult management problem, as it
  gets hard to figure out what to wrap and unwrap.

* Create processing pipelines, where work requests are placed on a
  queue, and the worker handlers process those requests. This seems
  like a good idea, in general, to allow distributed AtomSpace
  processing.  The AtomSpace does not currently have such a mechanism,
  and it seems like it should. However, building a generic mechanism
  of this kind seems like a bigger task than just the proxying
  requirement above: it risks getting complicated and heavyweight.
  This is not considered further here (but should be considered
  elsewhere)

* Capture the Atom read/write requests at the Sexpr shell level. The
  CogStorageNode uses the sexpr shell to communicate with the CogServer.
  Everything that the (remote) CogStorageNode sees passes through the
  sexpr shell.  This shell is "simple": it provides about 16 commands
  to support this communication. Each command is a UTF8 string, and is
  easily dispatched to a handler that performs that action.  The default
  command processor is in
  [Commands.h](https://github.com/opencog/atomspace/tree/master/opencog/persist/sexpr/Commands.h)
  in the AtomSpace git repo. Those commands can be intercepted in a very
  straight-forward way, and alternative actions can be performed when
  they arrive.

This third design choice is what is made here.

Coordination and Configuration
------------------------------
There's a bunch of confusing ideas about how the client and server
should coordinate with regards to what kind of proxying should be done.

* Should the client be able to tell the CogServer what kind of proxying
  it wants? How? What if different clients ask the same server for
  different things?

* How to expose different proxies? Consider a write-thru proxy `wthru`:

  * Alter `CogStorageNode` to detect URL's of the form
    `wthru://example.com/xxx` and use that to open a CogServer
    connection to `wthru` instead of `sexpr`. On the CogServer
    side, there is a `libwthru-shell.so` that responds to this.

  * Use URL's of the form `cog://example.com?policy=w-thru&foo=bar`.
    The CogStorageNode splits off everything following the `?` and
    uses that to configure the `sexpr` shell. This is what is
    currently implemented.

  * Manual configuration.  See below for examples and details.

  * Create a `WriteThroughStorageNode` that negotiates with the
    CogServer to do the right thing... This could be as simple as
    a silly little wrapper around `CogStorageNode`. In principle,
    this is more powerful, since it gives AtomSpace processes control
    over the configuration. In practice, using different URL's is
    acceptable, for now.

* How to configure the proxy itself? Consider a write-thru proxy:

  * There can be a no-configuration mode: on startup, the proxy looks
    for all open StorageNodes in the AtomSpace, and then always performs
    writes through those nodes.  Thus, the CogServer user must, prior
    to using the proxy, create the needed StorageNodes, and open them,
    and leave them open (for writing.)  As of right now, this is what
    is coded up (because its easy.)

  * To configure the proxy, there are two ways to do this. One is to
    use the CogServer `config` command. Yuck.  The other is to use
    `(use-modules (opencog cogserver))` and pass configuration into
    through that module.  This has the advantage (and disadvantage)
    that only the person setting up the cogserver can configure it.

  * For a configurable write-thru proxy, the config is presumably a
    list of StorageNode to write to.


Read-Thru and Write-Thru Proxy Agents
-------------------------------------
The read-thru and write-thru proxy agents can be manually loaded.
These can be used together, or separately. If used separately,
then only the reading resp. writing pass-thru functions are available.

The example below starts by connecting to the cogserver; poking around,
then loading the proxies, then looking around some more.
```
$ rlwrap telnet localhost 17001
opencog> list
   Module Name           Library            Module Directory Path
   -----------           -------            ---------------------
BuiltinRequestsModule libbuiltinreqs.so  /usr/local/lib/opencog/modules
SexprShellModule      libsexpr-shell.so  /usr/local/lib/opencog/modules
...

opencog> config SexprShellModule libr-thru-proxy.so
opencog> config SexprShellModule libw-thru-proxy.so
opencog> list
   Module Name           Library            Module Directory Path
   -----------           -------            ---------------------
ReadThruProxy         libr-thru-proxy.so  /usr/local/lib/opencog/modules
WriteThruProxy        libw-thru-proxy.so  /usr/local/lib/opencog/modules
...
```
That's it. After this point, all subsequent `sexpr` invocations will
pass through both of the pass-thru proxies.  Alternately, just use the
`cog://example.com?r-thru&w-thru` URL in the CogStorageNode.

Design Notes
------------
Here's how it actually works. It's a bunch of stovepipe code:

The file `Commands.cc` allows different command dispatchers to be
installed. A command dispatcher has the form

```
    std::string SomeClass:do_whatever(const std::string&)
```
which is installed by saying
```
    SexprEval* sev = ...
    sev->install_handler("cog-set-foo",
        std::bind(&SomeClass:do_whatever, this, _1));
```
After installation, whenever a network client sends the string
`(cog-set-foo stuffs)` the string `stuffs)` will be passed to the
method `SomeClass:do_whatever`. Then, whatever string is returned,
that string is sent back to the client. The list of the actual set
of command strings that are currently in use can be found in the
`Commands.cc` file.

TODO
----
Simple todo items:

* Expand the `WriteThruProxyUnitTest` to test the other four
  handlers. Also, write unit tests that use the `CogStorageNode`
  to make sure it works end-to-end.  Tedious and time-consuming.

* Create a `ReadThruProxyUnitTest`.

* Test both, together.

All of the above is "easy to do" but tedious and time-consuming.
See also the TODO list at the bottom, for the more abstract and more
difficult work items. Those will be hard.

### Complex TODO
 * SpaceFrames need to be handled in the StorageNodes!
 * The support for SpaceFrames by the CogStorageNode is currently
   incomplete and broken.
 * Implement a caching read-through node, so that read requests hit
   the disk only the first time, and subsequent reads do not go to disk
   a second time.

-----

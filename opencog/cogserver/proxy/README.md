Proxy Agents
------------
The code here implements an under-construction, experimental form
of AtomSpace proxying; that is, of receiving Atoms over the network,
and passing them onwards to other StorageNodes.

In the standard mode, when a network connection is made to the
CogServer, the user at the far end of the network connection is working
with the AtomSpace that this CogServer holds. It is reasonable to want
to have the CogServer attached to storage (say, disk storage), so that
when an Atom is received, it is also written to disk.  This general
idea is called "proxying", and "proxy agents" are responsible to doing
whatever needs to be done, when a read or write request for an Atom is
received by the CogServer. The proxy agent passes on those rquests to
other StorageNodes.

The diagram below shows a typical usage scenario. In this example,
the Link Grammar parser is reading from the local AtomSpace to which
it is attached. That AtomSpace uses a CogStorageNode to get access
to the "actual data", residing on the CogServer. However, the CogServer
itself doesn't "have the data"; its actually on disk (in the
RocksStorageNode) Thus, any read requests made by the the LG parser
have to be proxied by the CogServer to the disk storaage (using a
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

Design Choices
--------------
How should the above be implemented?  There are several design choices.

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
  and it seems like it should. However, building a generic mechnism
  of this kind seems like a bigger task than just the proxying
  requirement above: it risks getting complicated and hevyweight.
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

  * Use URL's of the form `cog://example.com?policy=wthru&foo=bar`.
    The CogStoragenode splits off everything following the `?` and
    uses that to configure the `sexpr` shell.

  * Manual configuration. This is what is being done right now.
    See below for details.



Write-Thru Proxy Agent
----------------------
The write-thru proxy agent is (manually) configured as follows:
```
opencog> list
   Module Name           Library            Module Directory Path
   -----------           -------            ---------------------
BuiltinRequestsModule libbuiltinreqs.so  /usr/local/lib/opencog/modules
SexprShellModule      libsexpr-shell.so  /usr/local/lib/opencog/modules
...

opencog> config SexprShellModule libwthru-proxy.so
opencog> list
   Module Name           Library            Module Directory Path
   -----------           -------            ---------------------
WriteThruProxy        libwthru-proxy.so  /usr/local/lib/opencog/modules
...
```
That's it. After this point, all subsequent `sexpr` invocations will
pass through the the write-thru proxy.


### TODO
* SpaceFrames need to be handled in the StorageNodes!

Design Notes
------------
To make proxy agents work, the cogserver needs to intercept sexpr
commands, and do non-default things with them.  There are several
things that needs to happen.

* Change the sexpr `Commands.cc` and provide a way of installing
  alternative handlers. (DONE)

* Provide a client with some way of stating what policy should be used.

How can the second thing be done?  There are two choices:

* Create a `WriteThruStorageNode` class, which is identical to the
  `CogStorageClass`, except it causes this policy agent to run.

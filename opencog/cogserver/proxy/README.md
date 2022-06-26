Write-Thru Proxy Agent
----------------------
The code here implements an under-construction, experimental form
of AtomSpace proxying; that is, of receiving Atoms over the network,
and passing them onwards to other StorageNodes.

In the standard mode, when a network connection is made to the
CogServer, the user at the far end of the network connection is working
with the AtomSpace that this CogServer holds. It is reasonable to ask
that the CogServer should be attached to storage (say, disk storage),
so that when an Atom is received, it is also written to disk. But how?
This general idea is called "proxying", and "proxy agents" are
responsible to doing whatever needs to be done, when an Atom is received
by the CogServer.

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
  Blech.

* Alter `CogStorageNode` to detect URL's of the form
  `wthru://example.com/xxx` and use that to open a Cogserver connection
  to `wthru` instead of `sexpr`.  Alternately, the URL could be
  `cog://example.com?policy=wthru&foo=bar` which is more flexible for
  future expansion.

The second alternative seems better.

There's yet another possibility: Alter `CogStorageNode` as above, and
pass the policy to the `sexpr` shell, via commands.  The downside to
this is that code lives in the AtomSpace git repo, and so we have a
chicken-n-egg problem in compiling it.

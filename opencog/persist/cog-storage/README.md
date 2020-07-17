
CogServer AtomSpace Server
--------------------------
Allow a CogServer to share it's AtomSpace with other AtomSpaces on
other machines. In ascii-art:

```
 +-------------+
 |             |
 |  CogServer  |  <-----internet------> Remote AtomSpace A
 |             |  <---+
 +-------------+      |
                      +-- internet ---> Remote AtomSpace B

```

Here, AtomSpace A can load/store Atoms (and Values) to the CogServer,
as can AtomSpace B, and so these two can share AtomSpace contents
however desired.

This provides a very simple, low-brow backend for AtomSpace storage
via the CogServer. At this time, it is ... not optimized for speed,
and its super-simplistic. It won't scale well. It is meant only as
a workbench for more general distributed strategies and experiments.

Example Usage
-------------

* Start the CogServer at "example.com":
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog cogserver))
scheme@(guile-user)> (start-cogserver)
$1 = "Started CogServer"
scheme@(guile-user)> Listening on port 17001
```
Then create some atoms (if desired)

* On the client machine:
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog persist))
scheme@(guile-user)> (use-modules (opencog persist-cog))
scheme@(guile-user)> (cogserver-open "cog://example.com/")
```

For more detailed examples, see the `examples` directory.

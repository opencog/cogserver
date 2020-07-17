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

Start the Server
----------------
All of the examples assume you have a cogserver started. This can be
done as (for example):
```
$ guile
scheme@(guile-user)> (use-modules (opencog))
scheme@(guile-user)> (use-modules (opencog cogserver))
scheme@(guile-user)> (start-cogserver)
$1 = "Started CogServer"
scheme@(guile-user)> Listening on port 17001
```

You can also do this in python.

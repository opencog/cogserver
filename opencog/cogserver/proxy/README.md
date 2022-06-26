
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

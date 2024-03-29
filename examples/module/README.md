
Example Module
---------------

In OpenCog, a "Module" is a dynamically loadable module: a wrapper for
some code that can be loaded into the CogServer after it has been
started.  The point of using a module is that it makes compiling and
debugging code easier, since, after a change, only the module needs to
be rebuilt, and not the entire cogserver.

This directory provides an example of a very basic module.

To test this module, do the following:

* Start the cogserver: From a shell prompt, issue the command
  `./opencog/cogserver/server/cogserver -c ../lib/development.conf`
   from the build directory

* Connect to the server: `telnet localhost 17001`

* Load the module: at the cogserver prompt, issue the command:
  `opencog> loadmodule examples/module/libexample_module.so`
  You should see 'done' printed on a line by itself.

* Verify that it has been loaded: at the cogserver prompt, issue the
  command `listmodules`. The module will be called
  `opencog::ExampleModule`, and so you should see:
  `Filename: libexample_module.so, ID: opencog::ExampleModule`

* Unload the module:
  `opencog> unloadmodule opencog::ExampleModule`

That's all folks!  The rest is up to you!

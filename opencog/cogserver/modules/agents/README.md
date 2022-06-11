MindAgents
==========
The MindAgents were an experimental job scheduling system for the
CogServer. The idea was that multiple different "agents" (kernel
threads and/or cooperative threads) can run within the CogServer
address space, thus avoiding the overhead and penalty of network
access via client-server type API's. The MindAgents system is obsolete
and no longer supported. The code here has never worked well, and now
it has bit-rotted into irrelelvance. This directory will be removed
at some point in the future.

For almost all users, it will be a lot easier to either run Python
and/or Scheme (guile) jobs at the CogServer command prompt.  All
Python and Scheme commands run in thier own thread, but share a common
AtomSpace, thus allowing multiple users to use the AtomSpace at the same
time. This is direct and straightforward, and avoids all of the
complexity and fragility of the Agents interface.

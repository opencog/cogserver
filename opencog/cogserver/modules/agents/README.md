MindAgents
==========
The MindAgents are an experimental job scheduling system for the
CogServer. The idea is that multiple different "agents" (kernel
threads and/or cooperative threads) can run within the CogServer
address space, thus avoiding the overhead and penalty of network
access via client-server type API's.

As such, this works only as well as the most poorly-behaved Agent
that gets loaded: if an Agent scribbles to RAM, exhausts resources,
or just plain crashes, it will bring down the hole cogserver. Thus,
Agents are meant for advanced users who really need the high-speed
acccess to the AtomSpace, but are also careful enough to write
well-designed code.

For almost all users, it will be a lot easier to either run Python
and/or Scheme (guile) jobs at the CogServer command prompt.  All
Python and Scheme commands run in thier own thread, but share a common
AtomSpace, thus allowing multiple users to use the AtomSpace at the same
time. This is direct and straightforward, and avoids all of the
complexity and fragility of the Agents interface.

The Agents interface more or less works, but it is unmaintianed.
Maintainers encouraged to volunteer!

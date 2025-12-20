#! /usr/bin/env python3
#
# start_cogserver.py
#
"""
Example of how the cogserver can be started from python.
Once the server is started, it can be accessed via its TCP/IP ports.
In this example, some Atoms are created in the AtomSpace before the
cogserver is started. It could also be done the other way around:
the cogserver could have been started first; the order does not matter.

The cogserver will use the same AtomSpace as the python shell; thus,
the Atoms created here will be visible there, and vice-versa.

Once the cogserver is running, the AtomSpace contents are available
via websockets in JSON format. The python and scheme evaluators are
also available through websockets; and all three: python, scheme and json,
are available through telnet. See the html pages in the example
directory for more.
"""

from opencog.type_constructors import *
from opencog.cogserver import start_cogserver, stop_cogserver

print("\nCreating some Atoms in the AtomSpace...")
A = ConceptNode('Apple')
B = ConceptNode('Berry')
C = ConceptNode('Comestible')

EdgeLink(PredicateNode("is a"), ListLink(A, ConceptNode("fruit")))
EdgeLink(PredicateNode("has a"), ListLink(A, ConceptNode("stem")))
is_red = EdgeLink(PredicateNode("often colored"), ListLink(A, ConceptNode("red")))

print("Starting the CogServer ...")
server = start_cogserver(console_port=17300,    # Default is 17001
                         web_port=18381,        # Default is 18080
                         mcp_port=19999)        # Default is 18888

print(f"\nCogServer started: {server}")

print("\nYou can connect using:")
print("  rlwrap telnet localhost 17300")

print("\nand then try some commands:")
print("  help")
print("  stats")

print("\nTo view the AtomSpace contents, try this:")
print("Enter the scm shell with `scm` and then run")
print("  (cog-prt-atomspace)")
print("Exit the scheme shell with a single dot or a ^D")

print("\nAlternately, enter the python shell with `py` and then run")
print("  from opencog.type_constructors import get_thread_atomspace")
print("  print(list(get_thread_atomspace()))")

print("\nYou can create new Atoms in those shells; they will appear here")
print("because this AtomSpace and the CogServer AtomSpace are one and the same.")
print("If you run MCP, you can even ask Claude to create Atoms for you.")

print("\nWill now sleep for ten minutes; after this, the cogserver will stop")
import time
time.sleep(600)
stop_cogserver(server)
print("... Time's up! Goodbye!")

# THE END. That's All, Folks!

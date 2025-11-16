#! /usr/bin/env python3
#
# start_cogserver.py
#
"""
Example of how the cogserver can be started from python.
Once the server is started, it can be accessed via it's TCP/IP ports.
In this example, an AtomSpace will be created and some data poked into
it. Only afterwards will the cogserver be started. It could also be
done the other way around: the cogserver could have been started first;
the order does not matter.

The cogserver will use the same AtomSpace as the python shell; thus,
the Atoms created here will be visible there, and vice-versa.

Once the cogserver is running, The AtomSpace contents are available
via webspockets in JSON format. The python and scheme evaluators are
also avialbe through websockets; and all three: python, scheme and json,
are available through telnet. See the html pages in the example
directory for more.
"""

# First, import the usual AtomSpace python modules.
from opencog.atomspace import AtomSpace
from opencog.type_constructors import *

# Create an AtomSpace.
my_atomspace = AtomSpace()

# Tell the type constructors to use it.
set_thread_atomspace(my_atomspace)

print("\nCreating an AtomSpace and putting some data into it...")
A = ConceptNode('Apple')
B = ConceptNode('Berry')
C = ConceptNode('Comestible')

EdgeLink(PredicateNode("is a"), ListLink(A, ConceptNode("fruit")))
EdgeLink(PredicateNode("has a"), ListLink(A, ConceptNode("stem")))
is_red = EdgeLink(PredicateNode("often colored"), ListLink(A, ConceptNode("red")))

from opencog.cogserver import (
    start_cogserver,
    stop_cogserver,
    is_cogserver_running,
    get_server_atomspace
)

print("Starting the CogServer ...")
start_cogserver(atomspace=my_atomspace,
                console_port=17300,    # Default is 17001
                web_port=18381,        # Default is 18080
                mcp_port=19999)        # Default is 18888

print("\nYou can connect using:")
print("  rlwrap telnet localhost 17300")

print("\nand then try some commands:")
print("  help")
print("  stats")

print("\nTo view the AtomSpace contents, try this:")
print("Enter the scm shell with `scm` and then run")
print("  (cog-prt-atomspace)")
print("Exit the scheme shell with a single dot or a ^D")

print("\nAlternately, enter the pythonshell with `py` and then run")
print("  print(\"my atomspace=\", list(my_atomspace))")

print("\nYou can create new Atoms in those shells; they will appear here")
print("because this AtomSpace and the CofserverAtomSpace are one and the same.")
print("If you run MCP, you can even ask Claude to create Atoms for you.")

print("\nWill now sleep for ten minutes; after this, the cogserver will stop")
import time
time.sleep(600)
stop_cogserver()
print("... Times up! Goodbye!")

# THE END. That's All, Folks!

#! /usr/bin/env python3
#
# start_cogserver.py
#
"""
Example of how the cogserver can be started from python.
Once the server is started, it can be accessed via it's TCP/IP ports.
In this example, an AtomSpace will be created and some data poked into
it. Only afterwards will the cogserver be started. It could also be
doe the other way around: the cogserver could have been started first;
the order does not matter.

The cogserver will use the same AtomSpace as the python shell; thus,
the Atoms created here will be visible there, and vice-versa.
"""

# First, import the usual AtomSpace python modules.
from opencog.atomspace import AtomSpace
from opencog.type_constructors import *

# Create an AtomSpace.
my_atomspace = AtomSpace()

# Tell the type constructors to use it.
set_default_atomspace(my_atomspace)

# Poke some data into the AtomSpace
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

start_cogserver(atomspace=my_atomspace,
                console_port=17300,    # Default is 17001
                web_port=18381,        # Default is 18080
                mcp_port=19999)        # Default is 18888

print("\nYou can connect using:")
print("  rlwrap telnet localhost 17300")

# stop_cogserver()

# THE END. That's All, Folks!

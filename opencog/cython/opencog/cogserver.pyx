include "request.pyx"

from opencog.cogserver cimport cython_server_atomspace

# For the below to work, we need to put the atomspace.pxd file where
# cython can find it (viz. /usr/local/include/opencog/cython)
from opencog.atomspace cimport AtomSpace_factoid

def get_server_atomspace():
    casp = cython_server_atomspace()
    asp = AtomSpace_factoid(casp)
    return asp

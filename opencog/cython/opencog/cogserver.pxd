# CogServer extern declarations
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr
from opencog.atomspace cimport cAtomSpacePtr, cHandle, cAtom

cdef extern from "opencog/cogserver/atoms/CogServerNode.h" namespace "opencog":
    cdef cppclass cCogServerNode "opencog::CogServerNode":
        cAtomSpacePtr getAtomSpace()

    ctypedef shared_ptr[cCogServerNode] cCogServerNodePtr "opencog::CogServerNodePtr"
    cCogServerNodePtr CogServerNodeCast(const cHandle&)

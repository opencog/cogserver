# CogServer extern declarations
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr
from opencog.atomspace cimport cAtomSpacePtr, cHandle, cAtom

cdef extern from "opencog/cogserver/server/CogServer.h" namespace "opencog":
    cdef cppclass cCogServer "opencog::CogServer":
        cCogServer()
        void enableNetworkServer(int port)
        void enableWebServer(int port)
        void enableMCPServer(int port)
        void disableNetworkServer()
        void disableWebServer()
        void disableMCPServer()
        void serverLoop() nogil
        void stop()
        bint running()
        void loadModules()

cdef extern from "opencog/cogserver/atoms/CogServerNode.h" namespace "opencog":
    cdef cppclass cCogServerNode "opencog::CogServerNode" (cCogServer):
        cAtomSpacePtr getAtomSpace()

    ctypedef shared_ptr[cCogServerNode] cCogServerNodePtr "opencog::CogServerNodePtr"
    cCogServerNodePtr CogServerNodeCast(const cHandle&)

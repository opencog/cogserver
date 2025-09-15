# Import needed types from atomspace
from opencog.atomspace cimport cAtomSpacePtr

# CogServer extern declarations
cdef extern from "opencog/cogserver/server/CogServer.h" namespace "opencog":
    cdef cppclass cCogServer "opencog::CogServer":
        cCogServer()
        cCogServer(cAtomSpacePtr)
        void enableNetworkServer(int port)
        void enableWebServer(int port)
        void enableMCPServer(int port)
        void disableNetworkServer()
        void disableWebServer()
        void disableMCPServer()
        void serverLoop() nogil
        void stop()
        bint running()
        cAtomSpacePtr getAtomSpace()
        void setAtomSpace(const cAtomSpacePtr&)
        void loadModules()

    # Singleton functions
    cCogServer& cogserver()
    cCogServer& cogserver(cAtomSpacePtr)
    cAtomSpacePtr cython_server_atomspace()

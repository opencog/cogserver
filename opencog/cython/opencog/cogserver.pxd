# CogServer extern declarations
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

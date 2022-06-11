
# Basic wrapping for std::string conversion
cdef extern from "<string>" namespace "std":
    cdef cppclass string:
        string()
        string(char *)
        char * c_str()
        int size()

# Handle

cdef extern from "opencog/atoms/base/Handle.h" namespace "opencog":
    cdef cppclass cHandle "opencog::Handle":
        cHandle()
        bint operator==(cHandle h)
        bint operator!=(cHandle h)
        bint operator<(cHandle h)
        bint operator>(cHandle h)
        bint operator<=(cHandle h)
        bint operator>=(cHandle h)
        cHandle UNDEFINED
# HandleSeq
    cdef cppclass cHandleSeq "opencog::HandleSeq"

# AtomSpaces
from opencog.atomspace cimport cAtomSpace
cdef extern from "opencog/cogserver/server/CogServer.h" namespace "opencog":
    cAtomSpace& cython_server_atomspace()

# ideally we'd import these typedefs instead of defining them here but I don't
# know how to do that with Cython
ctypedef short stim_t

cdef extern from "opencog/cogserver/server/Request.h" namespace "opencog":
    cdef cppclass cRequest "opencog::Request":
        void send(string s)

cdef class Request:
    cdef cRequest *c_obj

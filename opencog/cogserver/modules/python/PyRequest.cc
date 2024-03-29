#include "PyRequest.h"

#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

static bool module_initialized = false;

PyRequest::PyRequest(CogServer& cs,
                     const std::string& moduleName,
                     const std::string& className,
                     RequestClassInfo* cci) :
    Request(cs)
{
    _cci = cci;

    _moduleName = moduleName;
    _className = className;

    // NOTE: You need to call the import functions once and only once
    // in each separate shared library that accesses Cython defined api.
    // If you don't then you get a crash when you call an api function.
    if (!module_initialized)
    {
        module_initialized = true;
    }
    
#if DEAD_CODE
    // Call out to our helper module written in cython  NOTE: Cython api calls
    // defined "with gil" can be called without grabbing the GIL manually.
    _pyrequest = instantiate_request(moduleName, className, this);

    if (_pyrequest == Py_None)
        logger().error() << "Error creating python request "
                         << _moduleName << "." << _className;
    else
        logger().info() << "Created python request "
                        << _moduleName << "." << _className;
#endif
}

PyRequest::~PyRequest()
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure(); 
    // decrement python object reference counter
    Py_DECREF(_pyrequest);
    PyGILState_Release(gstate); 
}

bool PyRequest::execute()
{
#if DEAD_CODE
    std::string result = run_request(_pyrequest, _parameters,
                                    _cogserver.getAtomSpace());
    // errors only with result is not empty... && duplicate
    // errors are not reported.
    if (result.size() > 0 && result != _last_result) {
        // Any string returned is a traceback
        logger().error("[%s::execute] Python Exception:\n%s",
                this->info().id.c_str(),result.c_str());
        _last_result = result;
        return false;
    }
    _last_result = result;
#endif
    return true;
}


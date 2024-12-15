/*
 * opencog/cogserver/modules/python/PythonModule.cc
 *
 * Copyright (C) 2013 by OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#include <opencog/cython/PyIncludeWrapper.h>
#include <opencog/cython/PythonEval.h>

#include <opencog/util/Config.h>
#include <opencog/util/misc.h>
#include <opencog/atomspace/AtomSpace.h>

#include "PyRequest.h"
#include "PythonModule.h"

using std::vector;
using std::string;

using namespace opencog;

//#define DPRINTF printf
#define DPRINTF(...)

DECLARE_MODULE(PythonModule);

Request* PythonRequestFactory::create(CogServer& cs) const
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    logger().info() << "Creating python request " << _pySrcModuleName << "." << _pyClassName;
    PyRequest* pma = new PyRequest(cs, _pySrcModuleName, _pyClassName, _cci);

    PyGILState_Release(gstate);
    return pma;
}

// ------

PythonModule::PythonModule(CogServer& cs) : Module(cs)
{
}

static bool already_loaded = false;

PythonModule::~PythonModule()
{
    logger().info("[PythonModule] destructor");

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    unregisterRequests();
    do_load_py_unregister();

    for (PythonRequestFactory* rf : _requestFactories) delete rf;

    already_loaded = false;

    PyGILState_Release(gstate);
}

bool PythonModule::unregisterRequests()
{
    // Requires GIL
    for (std::string s : _requestNames) {
        DPRINTF("Unregistering requests of id %s\n", s.c_str());
        _cogserver.unregisterRequest(s);
    }

    return true;
}

void PythonModule::init()
{
    // Avoid hard crash if already loaded.
    if (already_loaded) return;
    already_loaded = true;

    logger().info("[PythonModule] Initialising Python CogServer module.");

    global_python_initialize();

    // Make sure that Python has been properly initialized.
    if (not Py_IsInitialized() /* || not PyEval_ThreadsInitialized() */) {
            throw opencog::RuntimeException(TRACE_INFO,
                    "Python not initialized, missing global_python_init()");
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    // Many python libraries (e.g. ROS) expect sys.argv to be set. So,
    // avoid the error print, and let them know who we are.
    PyRun_SimpleString("import sys; sys.argv='cogserver'\n");

    if (opencog::config().has("PYTHON_PRELOAD")) preloadModules();
    do_load_py_register();

    PyGILState_Release(gstate);
}

bool PythonModule::preloadModules()
{
    // requires GIL
    std::vector<std::string> pythonmodules;
    tokenize(opencog::config()["PYTHON_PRELOAD"], std::back_inserter(pythonmodules), ", ");
    for (std::vector<std::string>::const_iterator it = pythonmodules.begin();
         it != pythonmodules.end(); ++it) {
        std::list<std::string> args;
        args.push_back(*it);
        logger().info("[PythonModule] Preloading python module " + *it);
        std::string result = do_load_py(NULL,args);
        logger().info("[PythonModule] " + result);
    }
    return true;
}

/// do_load_py -- load python code, given a file name. (Implements the
/// loadpy command)
///
/// It is expected that the file contains a python module. The module
/// should contain a 'request' (shell command, written in python).
/// Requests/commnds must inherit from opencog.cogserver.Request
//
std::string PythonModule::do_load_py(Request *dummy, std::list<std::string> args)
{
    if (args.empty()) return "Please specify Python module to load.";
    std::string moduleName = args.front();

    std::ostringstream oss;
    if (moduleName.substr(moduleName.size()-3,3) == ".py") {
        oss << "Warning: Python module name should be "
            << "passed without .py extension" << std::endl;
        moduleName.replace(moduleName.size()-3,3,"");
    }

#if DEAD_CODE
    // Load the commands/requests, if any.
    requests_t thingsInModule = load_req_module(moduleName);
    if (thingsInModule.requests.size() > 0) {
        bool first = true;
        oss << "Python Requests found: ";
        for (size_t i=0; i< thingsInModule.requests.size(); i++) {
            std::string s = thingsInModule.requests[i];
            std::string short_desc = thingsInModule.req_summary[i];
            std::string long_desc = thingsInModule.req_description[i];
            bool is_shell = thingsInModule.req_is_shell[i];

            // CogServer commands in Python
            // Register request with cogserver using dotted name:
            // module.RequestName
            std::string dottedName = moduleName + "." + s;
            // register a custom factory that knows how to
            PythonRequestFactory* fact =
                new PythonRequestFactory(moduleName, s, short_desc, long_desc, is_shell);
            _requestFactories.push_back(fact);
            _cogserver.registerRequest(dottedName, fact);
            // save a list of Python requests that we've added to the CogServer
            _requestNames.push_back(dottedName);

            if (!first) { oss << ", "; first = false; }
            oss << s;
        }
        oss << ".\n";
    } else {
        oss << "No subclasses of opencog.cogserver.Request found.\n";
    }
#endif

    // Return info on what requests were found
    // This gets printed out to the user at the shell prompt.
    return oss.str();
}

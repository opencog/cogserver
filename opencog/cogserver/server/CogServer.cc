/*
 * opencog/cogserver/server/CogServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/time.h>

#include <boost/filesystem/operations.hpp>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/files.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include <opencog/atomspace/AtomSpace.h>

#include <opencog/cogserver/server/ServerConsole.h>
#include <opencog/cogserver/network/NetworkServer.h>
#include <opencog/cogserver/server/Request.h>

#include "CogServer.h"
#include "BaseServer.h"

using namespace opencog;

BaseServer* CogServer::createInstance(AtomSpace* as)
{
    return new CogServer(as);
}

CogServer::~CogServer()
{
    logger().debug("[CogServer] enter destructor");
    disableNetworkServer();

    std::vector<std::string> moduleKeys;

    for (ModuleMap::iterator it = modules.begin(); it != modules.end(); ++it)
        moduleKeys.push_back(it->first);

    // unload all modules
    for (std::vector<std::string>::iterator k = moduleKeys.begin(); k != moduleKeys.end(); ++k) {
        // retest the key because it might have been removed already
        ModuleMap::iterator it = modules.find(*k);
        if (it != modules.end()) {
            logger().debug("[CogServer] removing module \"%s\"", it->first.c_str());
            ModuleData mdata = it->second;

            // cache filename and id to erase the entries from the modules map
            std::string filename = mdata.filename;
            std::string id = mdata.id;

            // invoke the module's unload function
            (*mdata.unloadFunction)(mdata.module);

            // erase the map entries (one with the filename as key, and
            // one with the module id as key)
            modules.erase(filename);
            modules.erase(id);
        }
    }

    logger().debug("[CogServer] exit destructor");
}

CogServer::CogServer(AtomSpace* as) :
    BaseServer(as),
    _networkServer(nullptr),
    _running(false)
{
}

void CogServer::enableNetworkServer(int port)
{
    if (_networkServer) return;
    _networkServer = new NetworkServer(config().get_int("SERVER_PORT", port));

    auto make_console = [](void)->ConsoleSocket*
	     { return new ServerConsole(); };
    _networkServer->run(make_console);
    _running = true;
}

void CogServer::disableNetworkServer()
{
    stop();
    if (_networkServer)
    {
        delete _networkServer;
        _networkServer = nullptr;
    }
}

void CogServer::stop()
{
    // Prevent the Network server from accepting any more connections,
    // and from queing any more Requests. I think. This might be racey.
    if (_networkServer) _networkServer->stop();

    _running = false;

    // Drain whatever is left in the queue.
    while (0 < getRequestQueueSize())
        processRequests();
}

void CogServer::serverLoop()
{
    logger().info("Starting CogServer loop.");
    while(_running)
    {
        while (0 < getRequestQueueSize())
            runLoopStep();

        // XXX FIXME. terrible terrible hack. What we should be
        // doing is running in our own thread, waiting on a semaphore,
        // until some request is queued. Spinning is .. just wrong.
        usleep(20000);
    }

    // No way to process requests. Stop accepting network connections.
    disableNetworkServer();
}

void CogServer::runLoopStep(void)
{
    // Process requests
    if (0 < getRequestQueueSize())
    {
        struct timeval timer_start, timer_end, requests_time;
        gettimeofday(&timer_start, NULL);
        processRequests();
        gettimeofday(&timer_end, NULL);
        timersub(&timer_end, &timer_start, &requests_time);

        logger().fine("[CogServer::runLoopStep] Time to process requests: %f",
                   requests_time.tv_usec/1000000.0
                  );
    }
}

void CogServer::processRequests(void)
{
    std::lock_guard<std::mutex> lock(processRequestsMutex);
    while (0 < getRequestQueueSize()) {
        Request* request = popRequest();
        request->execute();
        delete request;
    }
}

// =============================================================
// Request registration

bool CogServer::registerRequest(const std::string& name,
                                AbstractFactory<Request> const* factory)
{
    return Registry<Request>::register_(name, factory);
}

bool CogServer::unregisterRequest(const std::string& name)
{
    return Registry<Request>::unregister(name);
}

Request* CogServer::createRequest(const std::string& name)
{
    return Registry<Request>::create(*this, name);
}

const RequestClassInfo& CogServer::requestInfo(const std::string& name) const
{
    return static_cast<const RequestClassInfo&>(Registry<Request>::classinfo(name));
}

std::list<const char*> CogServer::requestIds() const
{
    return Registry<Request>::all();
}

// =============================================================
// Module handling

bool CogServer::loadModule(const std::string& filename)
{
// TODO FIXME I guess this needs to be different for windows.
#define PATH_SEP '/'
    // The module file identifier does NOT include the file path!
    std::string fi = filename;
    size_t path_sep = fi.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        fi.erase(0, path_sep+1);
    if (modules.find(fi) !=  modules.end()) {
        logger().info("Module \"%s\" is already loaded.", fi.c_str());
        return true;
    }

    // reset error
    dlerror();

    logger().info("Loading module \"%s\"", filename.c_str());
#ifdef __APPLE__
    // Tell dyld to search runpath
    std::string withRPath("@rpath/");
    withRPath += filename;
    // Check to see if so extension is specified, replace with .dylib if it is.
    if (withRPath.substr(withRPath.size()-3,3) == ".so") {
        withRPath.replace(withRPath.size()-3,3,".dylib");
    }
    void *dynLibrary = dlopen(withRPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#else
    void *dynLibrary = dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
    const char* dlsymError = dlerror();
    if ((dynLibrary == NULL) || (dlsymError)) {
        // This is almost surely due to a user configuration error.
        // User errors are always logged as warnings.
        logger().warn("Unable to load module \"%s\": %s", filename.c_str(), dlsymError);
        return false;
    }

    // reset error
    dlerror();

    // search for id function
    Module::IdFunction* id_func =
	    (Module::IdFunction*) dlsym(dynLibrary, Module::id_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"opencog_module_id\": %s (module %s)", dlsymError, filename.c_str());
        return false;
    }

    // get and check module id
    const char *module_id = (*id_func)();
    if (module_id == NULL) {
        logger().warn("Invalid module id (module \"%s\")", filename.c_str());
        return false;
    }

    // search for 'load' & 'unload' symbols
    Module::LoadFunction* load_func =
	    (Module::LoadFunction*) dlsym(dynLibrary, Module::load_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"opencog_module_load\": %s", dlsymError);
        return false;
    }

    Module::UnloadFunction* unload_func =
	    (Module::UnloadFunction*) dlsym(dynLibrary, Module::unload_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"opencog_module_unload\": %s",
                       dlsymError);
        return false;
    }

    // load and init module
    Module* module = (Module*) (*load_func)(*this);

    // store two entries in the module map:
    //    1: filename => <struct module data>
    //    2: moduleid => <struct module data>
    // we rely on the assumption that no module id will match the filename of
    // another module (and vice-versa). This is probably reasonable since most
    // module filenames should have a .dll or .so suffix, and module ids should
    // (by convention) be prefixed with its class namespace (i.e., "opencog::")
    std::string i = module_id;
    std::string f = filename;
    // The filename does NOT include the file path!
    path_sep = f.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        f.erase(0, path_sep+1);
    ModuleData mdata = {module, i, f, load_func, unload_func, dynLibrary};
    modules[i] = mdata;
    modules[f] = mdata;

    // after registration, call the module's init() method
    module->init();

    return true;
}

std::string CogServer::listModules()
{
    // Prepare a stream to collect the module information
    std::ostringstream oss;

    // Prepare iterators to process the ModuleMap
    ModuleMap::iterator startIterator = modules.begin();
    ModuleMap::iterator endIterator = modules.end();

    // Loop through the ModuleMap
    for(; startIterator != endIterator; ++startIterator)
    {
        // Get the module_id from the item
        std::string module_id = startIterator->first;
        ModuleData moduleData = startIterator->second;

        // Only list the names, not the filenames.
        if (module_id.find(".so", 0) != std::string::npos)
        {
            // Add the module_id to our stream
            oss
            // << "ModuleID: " << module_id
            << "Filename: " << moduleData.filename
            << ", ID: " << moduleData.id
            // << ", Load function: " << moduleData.loadFunction
            // << ", Module: " << moduleData.module
            // << ", Unload function: " << moduleData.unloadFunction
            << std::endl;
        }

    }

    // Return the contents of the stream
    return oss.str();
}

bool CogServer::unloadModule(const std::string& moduleId)
{
    // The module file identifier does NOT include the file path!
    std::string f = moduleId;
    size_t path_sep = f.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        f.erase(0, path_sep+1);
    logger().info("[CogServer] unloadModule(%s)", f.c_str());
    ModuleMap::const_iterator it = modules.find(f);
    if (it == modules.end()) {
        logger().info("[CogServer::unloadModule] module \"%s\" is not loaded.", f.c_str());
        return false;
    }
    ModuleData mdata = it->second;

    // cache filename, id and handle
    std::string filename = mdata.filename;
    std::string id       = mdata.id;
    void*       handle   = mdata.handle;

    // invoke the module's unload function
    (*mdata.unloadFunction)(mdata.module);

    // erase the map entries (one with the filename as key, and one with the module
    // id as key
    modules.erase(filename);
    modules.erase(id);

    // unload dynamically loadable library
    logger().info("Unloading module \"%s\"", filename.c_str());

    dlerror(); // reset error
    if (dlclose(handle) != 0) {
        const char* dlsymError = dlerror();
        if (dlsymError) {
            logger().warn("Unable to unload module \"%s\": %s", filename.c_str(), dlsymError);
            return false;
        }
    }

    return true;
}

CogServer::ModuleData CogServer::getModuleData(const std::string& moduleId)
{
    // The module file identifier does NOT include the file path!
    std::string f = moduleId;
    size_t path_sep = f.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        f.erase(0, path_sep+1);

    ModuleMap::const_iterator it = modules.find(f);
    if (it == modules.end()) {
        logger().info("[CogServer::getModuleData] module \"%s\" was not found.", f.c_str());
        ModuleData nulldata = {NULL, "", "", NULL, NULL, NULL};
        return nulldata;
    }
    return it->second;
}

Module* CogServer::getModule(const std::string& moduleId)
{
    return getModuleData(moduleId).module;
}

void CogServer::loadModules(std::vector<std::string> module_paths)
{
    if (module_paths.empty())
    {
        // XXX FIXME This hack allows an installed cogserver to
        // inadvertantly load from a fixed build path. This should
        // be handled by the config file or test environment...
        // not hard coded.
        // Give priority search order to the build directories
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/agents");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/commands");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/python");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/server/");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/shell/");

        // If not found at above locations, search the install paths
        for (auto p : get_module_paths())
        {
            module_paths.push_back(p);
            module_paths.push_back(p + "/opencog");
            module_paths.push_back(p + "/opencog/modules");
        }
    }

    // Load modules specified in the config file
    std::string modlist;
    if (config().has("MODULES"))
        modlist = config().get("MODULES");
    else
        // Defaults: search the build dirs first, then the install dirs.
        modlist =
            "libbuiltinreqs.so, "
            "libscheme-shell.so, "
            "libpy-shell.so";
    std::vector<std::string> modules;
    tokenize(modlist, std::back_inserter(modules), ", ");
    bool load_failure = false;
    for (const std::string& module : modules) {
        bool rc = false;
        if (not module_paths.empty()) {
            for (const std::string& module_path : module_paths) {
                boost::filesystem::path modulePath(module_path);
                modulePath /= module;
                if (boost::filesystem::exists(modulePath)) {
                    rc = loadModule(modulePath.string());
                    if (rc) break;
                }
            }
        } else {
            rc = loadModule(module);
        }
        if (!rc)
        {
            logger().warn("Failed to load cogserver module %s", module.c_str());
				load_failure = true;
        }
    }
    if (load_failure) {
        for (auto p : module_paths)
            logger().warn("Searched for module at %s", p.c_str());
    }
}

Logger &CogServer::logger()
{
    return ::logger();
}

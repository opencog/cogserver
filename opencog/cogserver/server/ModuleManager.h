/*
 * opencog/module/ModuleManager.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_MODULE_MANAGER_H
#define _OPENCOG_MODULE_MANAGER_H

#include <map>
#include <vector>

#include <opencog/cogserver/server/Module.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * Module management is responsible for extending the server
 * through the use of dynamically loadable libraries (or modules).
 * Valid modules must extended the class defined in Module.h and be
 * compiled and linked as a shared library. Currently, only Unix DSOs
 * are supported; Win32 DLLs are not. The server API itself provides
 * methods to load, unload and retrieve  modules. The server provides
 * modules with two entry points: the constructor, which is typically
 * invoked by the module's load function; and the 'init' method, which
 * is called after the module has been instantiated and its meta-data 
 * has been filled.
 */
class ModuleManager
{
protected:

    // Define a map with the list of loaded modules.
    typedef struct {
        Module*                 module;
        std::string             id;
        std::string             filename;
        Module::LoadFunction*   loadFunction;
        Module::UnloadFunction* unloadFunction;
        void*                   handle;
    } ModuleData;

    // Container used to store references to the modules.
    typedef std::map<const std::string, ModuleData> ModuleMap;
    ModuleMap modules;

public:

    /** ModuleManager's constructor. */
    ModuleManager(void);

    /** ModuleManager's destructor. Unloads all modules. */
    ~ModuleManager(void);

    /** Loads a dynamic library/module. Takes the filename of the
     *  library (.so or .dylib or .dll). On Linux/Unix, the filename may
     *  be absolute or relative to the server's RPATH path (which
     *  typically, should be "INSTALL_PREFIX/lib/opencog") */
    bool loadModule(const std::string& filename, CogServer&);

    /** Unloads a dynamic library/module. Takes the module's id, as
     *  defined in the Module base class and overriden by the derived
     *  module classes. See the documentation in the Module.h file for
     *  more details. */
    bool unloadModule(const std::string& id);

    /** Lists the modules that are currently loaded. */
    std::string listModules();

    /** Retrieves the module's meta-data (id, filename, load/unload
     * function pointers, etc). Takes the module's id */
    ModuleData getModuleData(const std::string& id);

    /** Retrieves the module's instance. Takes the module's id */
    Module* getModule(const std::string& id);

    /** Load all modules specified in configuration file. If
        module_paths is empty then DEFAULT_MODULE_PATHS is used
        instead, which is why it is passed as copy instead of const
        ref. */
    void loadModules(std::vector<std::string> module_paths,
                             CogServer&);
    void loadModules(CogServer& cs) {
        loadModules(std::vector<std::string>(), cs);
    }

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_MODULE_MANAGER_H

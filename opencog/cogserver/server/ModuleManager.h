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
 * Cogserver modules provide a way for custom C++ code to get to the
 * network data that the cogserver is receiving, and to generate
 * output in response. Basically, all the complexities of socket
 * handling are abstracted away, allowing the module author to work
 * with data on a line-by-line basis.
 *
 * Modules are dynamically loadable shared libraries. Please use the
 * existing modules as examples for how to create new ones.  Most of
 * the existing modules are rather simple.
 *
 * Currently, only Unix shared libs (DSO's or Dynamically Shared
 * Objects) are supported; Win32 DLLs are not.
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

    /** Configure a dynamic library/module. Passes the given
     * configuration string to the module for processing. Returns
     * false if configuration failed, else returns true.
     */
    bool configModule(const std::string& id, const std::string& cfg);

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

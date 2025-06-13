/*
 * opencog/cogserver/server/ModuleManager.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * The Cogserver Module system is deprecated; users are encouraged to
 * explore writing guile (scheme) or python modules instead.
 */

#include <dlfcn.h>
#include <unistd.h>

#include <filesystem>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/files.h>
#include <opencog/util/misc.h>
#include <opencog/util/platform.h>

#include "ModuleManager.h"

using namespace opencog;

ModuleManager::ModuleManager(void)
{
    // Give priority search order to the build directories.
    // Do NOT search these, if working from installed path!
    std::string exe = get_exe_dir();
    if (0 == exe.compare(0, sizeof(PROJECT_BINARY_DIR)-1, PROJECT_BINARY_DIR))
    {

        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/commands");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/python");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/modules/");
        module_paths.push_back(PROJECT_BINARY_DIR "/opencog/cogserver/shell/");
    }
    module_paths.push_back(PROJECT_INSTALL_PREFIX "/lib/opencog/modules/");
}

ModuleManager::~ModuleManager()
{
    logger().debug("[ModuleManager] enter destructor");

    std::vector<std::string> moduleKeys;

    for (ModuleMap::iterator it = modules.begin(); it != modules.end(); ++it)
        moduleKeys.push_back(it->first);

    // unload all modules
    for (std::vector<std::string>::iterator k = moduleKeys.begin(); k != moduleKeys.end(); ++k) {
        // retest the key because it might have been removed already
        ModuleMap::iterator it = modules.find(*k);
        if (it != modules.end()) {
            logger().debug("[ModuleManager] removing module \"%s\"", it->first.c_str());
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

    logger().debug("[ModuleManager] exit destructor");
}

// ====================================================================

/// If `filepath` contains directory slashes, return only
/// the fragment of the string after the last slash.
static std::string get_filename(const std::string& fullpath)
{
#define PATH_SEP '/'
    size_t path_sep = fullpath.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        return fullpath.substr(path_sep+1);
    return fullpath;
}

/// Get the directory portion of the fullpath.
static std::string get_filepath(const std::string& fullpath)
{
    size_t path_sep = fullpath.rfind(PATH_SEP);
    if (path_sep != std::string::npos)
        return fullpath.substr(0, path_sep);
    return fullpath;
}

bool ModuleManager::loadAbsPath(const std::string& path,
                               CogServer& cs)
{
    std::string fi = get_filename(path);
    if (modules.find(fi) !=  modules.end()) {
        logger().info("Module \"%s\" is already loaded.", fi.c_str());
        return true;
    }

    // reset error
    dlerror();

    logger().info("Loading module \"%s\"", path.c_str());
#ifdef __APPLE__
    // Tell dyld to search runpath.
    std::string withRPath("@rpath/");
    withRPath += path;
    // Check to see if so extension is specified, replace with .dylib if it is.
    if (withRPath.substr(withRPath.size()-3,3) == ".so") {
        withRPath.replace(withRPath.size()-3,3,".dylib");
    }
    void *dynLibrary = dlopen(withRPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#else
    void *dynLibrary = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
    const char* dlsymError = dlerror();
    if ((dynLibrary == NULL) || (dlsymError)) {
        // This is almost surely due to a user configuration error.
        // User errors are always logged as warnings.
        logger().warn("Unable to load module \"%s\": %s",
                       path.c_str(), dlsymError);
        return false;
    }

    // Reset error state.
    dlerror();

    // Search for id function.
    Module::IdFunction* id_func =
        (Module::IdFunction*) dlsym(dynLibrary, Module::id_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"%s\": %s (module %s)",
                        Module::id_function_name(), dlsymError, path.c_str());
        return false;
    }

    // Get and check module id.
    const char *module_id = (*id_func)();
    if (module_id == NULL) {
        logger().warn("Invalid module id (module \"%s\")", path.c_str());
        return false;
    }

    // Search for 'load', 'unload' and 'config' symbols.
    Module::LoadFunction* load_func =
        (Module::LoadFunction*) dlsym(dynLibrary, Module::load_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"%s\": %s",
                       Module::load_function_name(), dlsymError);
        return false;
    }

    Module::UnloadFunction* unload_func =
        (Module::UnloadFunction*) dlsym(dynLibrary, Module::unload_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"%s\": %s",
                       Module::unload_function_name(), dlsymError);
        return false;
    }

    Module::ConfigFunction* config_func =
        (Module::ConfigFunction*) dlsym(dynLibrary, Module::config_function_name());
    dlsymError = dlerror();
    if (dlsymError) {
        logger().error("Unable to find symbol \"%s\": %s",
                       Module::config_function_name(), dlsymError);
        return false;
    }

    // Load and init module
    Module* module = (Module*) (*load_func)(cs);

    // Store two entries in the module map:
    //    1: filename => <struct module data>
    //    2: moduleid => <struct module data>
    // We rely on the assumption that no module id will match the
    // filename of another module (and vice-versa). This is probably
    // reasonable since most module filenames should have a .dll or
    // .dylib or .so suffix.
    std::string i = module_id;
    std::string f = get_filename(path);
    std::string p = get_filepath(path);
    ModuleData mdata = {module, i, f, p, load_func, unload_func,
                        config_func, dynLibrary};
    modules[i] = mdata;
    modules[f] = mdata;

    // after registration, call the module's init() method
    module->init();

    return true;
}

// ====================================================================

std::string ModuleManager::listModules()
{
    std::string rv =
        "   Module Name           Library            Module Directory Path\n"
        "   -----------           -------            ---------------------\n";
    for (const auto& modpr : modules)
    {
        // The list holds both lib.so's, and names.
        // Loop only over the so's.
        const std::string& module_id = modpr.first;
        if (module_id.find(".so", 0) == std::string::npos)
            continue;

        ModuleData mdata = modpr.second;

        // Truncate the filepath. Sorry! Is there a better way?
        std::string trunc = mdata.dirpath;
        size_t tlen = trunc.size();
        if (38 < tlen)
            trunc = "..." + trunc.substr(tlen-35);

        char buff[120];
        snprintf(buff, 120, "%-21s %-18s %s\n", mdata.id.c_str(),
                 mdata.filename.c_str(), trunc.c_str());
        rv += buff;
    }

    return rv;
}

// ====================================================================

bool ModuleManager::unloadModule(const std::string& moduleId)
{
    ModuleData mdata = getModuleData(moduleId);

    // Unable to find the module!
    if (nullptr == mdata.module) return false;

    // Cache filename, id and handle; we'll need these in just a moment.
    std::string filename = mdata.filename;
    std::string id       = mdata.id;
    void*       handle   = mdata.handle;

    // Invoke the module's unload function.
    (*mdata.unloadFunction)(mdata.module);

    // erase the map entries (one with the filename as key,
    // and one with the module id as key
    modules.erase(filename);
    modules.erase(id);

    // Unload dynamically loadable library.
    logger().info("Unloading module \"%s\"", filename.c_str());

    dlerror(); // Reset error state.
    if (dlclose(handle) != 0) {
        const char* dlsymError = dlerror();
        if (dlsymError) {
            logger().warn("Unable to unload module \"%s\": %s",
                           filename.c_str(), dlsymError);
            return false;
        }
    }

    return true;
}

// ====================================================================

bool ModuleManager::configModule(const std::string& moduleId,
                                 const std::string& cfg)
{
    ModuleData mdata = getModuleData(moduleId);

    // If the module isn't found ...
    if (nullptr == mdata.module) return false;

    // Invoke the module's config function.
    bool rc = (*mdata.configFunction)(mdata.module, cfg.c_str());

    return rc;
}

// ====================================================================

ModuleManager::ModuleData ModuleManager::getModuleData(const std::string& moduleId)
{
    std::string f = get_filename(moduleId);
    ModuleMap::const_iterator it = modules.find(f);
    if (it == modules.end()) {
        logger().info("[ModuleManager] module \"%s\" was not found.", f.c_str());
        static ModuleData nulldata = {NULL, "", "", "", NULL, NULL, NULL, NULL};
        return nulldata;
    }
    return it->second;
}

Module* ModuleManager::getModule(const std::string& moduleId)
{
    return getModuleData(moduleId).module;
}

// ====================================================================

bool ModuleManager::loadModule(const std::string& path,
                               CogServer& cs)
{
    if (0 == path.size()) return false;
    if ('/' == path[0])
        return loadAbsPath(path, cs);

    // Loop over the different possible module paths.
    bool rc = false;
    for (const std::string& module_path : module_paths) {
        std::filesystem::path modulePath(module_path);
        modulePath /= path;
        if (std::filesystem::exists(modulePath)) {
            rc = loadModule(modulePath.string(), cs);
            if (rc) break;
        }
    }
    return rc;
}

void ModuleManager::loadModules(CogServer& cs)
{
    // Load modules specified in the config file
    std::string modlist;
    if (config().has("MODULES"))
        modlist = config().get("MODULES");
    else
        // Defaults: search the build dirs first, then the install dirs.
        modlist =
            "libbuiltinreqs.so, "
            "libtop-shell.so, "
            "libscheme-shell.so, "
            "libsexpr-shell.so, "
            "libjson-shell.so, "
            "libmcp-shell.so, "
            "libpy-shell.so";

    std::vector<std::string> modules;
    tokenize(modlist, std::back_inserter(modules), ", ");
    bool load_failure = false;
    for (const std::string& module : modules) {
        bool rc = loadModule(module, cs);
        if (not rc)
        {
            logger().warn("Failed to load module %s", module.c_str());
            load_failure = true;
        }
    }
    if (load_failure) {
        for (auto p : module_paths)
            logger().warn("Searched for module at %s", p.c_str());
    }
}

// ========================= END OF FILE ==============================
// ====================================================================

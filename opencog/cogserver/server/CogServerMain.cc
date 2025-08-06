/*
 * opencog/cogserver/server/CogServerMain.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <getopt.h>
#include <langinfo.h>
#include <locale.h>
#include <signal.h>
#include <string.h>

#include <filesystem>
#include <string>
#include <thread>
#include <utility>

#include <boost/algorithm/string.hpp>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/files.h>
#include <opencog/util/misc.h>

#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

static const char* DEFAULT_CONFIG_FILENAME = "cogserver.conf";
static const char* DEFAULT_CONFIG_ALT_FILENAME = "opencog.conf";
static const char* DEFAULT_CONFIG_PATHS[] =
{
    // Search order for the config file:
    ".",         // First, we look in the current directory,
    "lib",       // Next, we look in the build directory (cmake puts it here)
    "../lib",    // Next, we look at the source directory
    CONFDIR,     // Next, the install directory
#ifndef WIN32
    "/etc",      // Finally, in the standard system directory.
#endif // !WIN32
    NULL
};

static void usage(const char* progname)
{
    std::cerr << "Usage: " << progname
        << " [-p <console port>] [-w <webserver port>] [-m <mcp port>] [-c <config-file>] [-DOPTION=\"VALUE\"]\n\n"
        << "If multiple config files are specified, then these are\n"
        << "loaded sequentially, with the values in later files\n"
        << "overwriting the earlier ones. -D Option values override\n"
        << "the options in config files."
        << "\n"
        << "Supported options and default values:\n"
        << "SERVER_PORT = 17001\n"
        << "WEB_PORT = 18080\n"
        << "MCP_PORT = 18888\n"
        << "CONFDIR = /usr/local/etc\n"
        << "LOG_FILE = /tmp/cogserver.log\n"
        << "LOG_LEVEL = info\n"
        << "LOG_TO_STDOUT = false\n"
        << "ANSI_PROMPT = ^[[0;32mopencog^[[1;32m> ^[[0m\n"
        << "PROMPT = opencog> \n"
        << "ANSI_SCM_PROMPT = ^[[0;34mguile^[[1;34m> ^[[0m\n"
        << "SCM_PROMPT = guile> \n"
        << "MODULES = libbuiltinreqs.so, ...\n"
        << std::endl;
}

// Catch and report sigsegv
void sighand(int sig)
{
    logger().set_print_to_stdout_flag(true);
    logger().error() << "Caught signal " << sig << " (" << strsignal(sig)
        << ") on thread " << std::this_thread::get_id();
    logger().flush();
    sleep(3);
    exit(1);
}

int main(int argc, char *argv[])
{
    // Get the locale from the environment...
    // Perhaps we should someday get it from the config file ???
    setlocale(LC_ALL, "");

    // Check to make sure the current locale is UTF8; if its not,
    // then force-set this to the english utf8 locale
    const char * codeset = nl_langinfo(CODESET);
    if (!strstr(codeset, "UTF") && !strstr(codeset, "utf"))
    {
       fprintf(stderr,
           "%s: Warning: locale %s was not UTF-8; force-setting to en_US.UTF-8\n",
            argv[0], codeset);
       setlocale(LC_CTYPE, "en_US.UTF-8");
    }

    int console_port = 17001;
    int webserver_port = 18080;
    int mcp_port = 18888;

    static const char *optString = "c:p:w:m:D:h";
    std::vector<std::string> configFiles;
    std::vector<std::pair<std::string, std::string>> configPairs;
    std::string progname = argv[0];

    // parse command line
    while (true) {
        int c = getopt (argc, argv, optString);
        /* Detect end of options */
        if (c == -1) {
            break;
        } else if (c == 'c') {
            configFiles.push_back(optarg);
        } else if (c == 'D') {
            // override all previous options, e.g.
            // -DLOG_TO_STDOUT=TRUE
            std::string text = optarg;
            std::vector<std::string> strs;
            boost::split(strs, text, boost::is_any_of("=:"));
            std::string optionName = strs[0];
            std::string value;
            if (strs.size() > 2) {
                // merge end tokens if more than one separator found
                for (uint i = 1; i < strs.size(); i++)
                    value += strs[i];
            } else if (strs.size() == 1) {
                std::cerr << "No value given for option "
                          << strs[0] << std::endl;
            } else {
                value = strs[1];
            }
            configPairs.push_back({optionName, value});
        } else if (c == 'p') {
            console_port = atoi(optarg);
        } else if (c == 'w') {
            webserver_port = atoi(optarg);
        } else if (c == 'm') {
            mcp_port = atoi(optarg);
        } else {
            // unknown option (or help)
            usage(progname.c_str());
            if (c == 'h')
                exit(0);
            else
                exit(1);
        }

    }

    // First, search for the standard config file.
    if (configFiles.size() == 0) {
        // search for configuration file on default locations
        for (int i = 0; DEFAULT_CONFIG_PATHS[i] != NULL; ++i) {
            std::filesystem::path configPath(DEFAULT_CONFIG_PATHS[i]);
            configPath /= DEFAULT_CONFIG_FILENAME;
            if (std::filesystem::exists(configPath)) {
                std::cerr << "Using default config at "
                          << configPath.string() << std::endl;
                configFiles.push_back(configPath.string());

                // Use the *first* config file found! We don't want to
                // load both the installed system config file, and also
                // any config file found in the build directory. We
                // ESPECIALLY don't want to load the system config file
                // after the development config file, thus clobbering
                // the contents of the devel config file!
                break;
            }
        }
    }

    // Next, search for alternate config file.
    if (configFiles.size() == 0) {
        // search for configuration file on default locations
        for (int i = 0; DEFAULT_CONFIG_PATHS[i] != NULL; ++i) {
            std::filesystem::path configPath(DEFAULT_CONFIG_PATHS[i]);
            configPath /= DEFAULT_CONFIG_ALT_FILENAME;
            if (std::filesystem::exists(configPath)) {
                std::cerr << "Using default config at "
                          << configPath.string() << std::endl;
                configFiles.push_back(configPath.string());

                // Use the *first* config file found! We don't want to
                // load both the installed system config file, and also
                // any config file found in the build directory. We
                // ESPECIALLY don't want to load the system config file
                // after the development config file, thus clobbering
                // the contents of the devel config file!
                break;
            }
        }
    }

    config().reset();
    if (configFiles.size() == 0) {
        std::cerr << "No config files could be found!" << std::endl;
        exit(-1);
    }

    // Each config file sequentially overwrites the next
    for (const std::string& configFile : configFiles) {
        try {
            config().load(configFile.c_str(), false);
            break;
        } catch (RuntimeException &e) {
            std::cerr << e.get_message() << std::endl;
            exit(1);
        }
    }

    // Each specific option
    for (const auto& optionPair : configPairs) {
        // std::cerr << optionPair.first << " = " << optionPair.second << std::endl;
        config().set(optionPair.first, optionPair.second);
    }

    if (config().has("LOG_LEVEL"))
        logger().set_level(config().get("LOG_LEVEL"));
    if (config().has("LOG_FILE"))
        logger().set_filename(config().get("LOG_FILE"));
    if (config().has("LOG_TO_STDOUT"))
    {
        std::string flg = config().get("LOG_TO_STDOUT");
        if (not ('f' == flg[0] or 'F' == flg[0] or '0' == flg[0]))
            logger().set_print_to_stdout_flag(true);
    }

    // Start catching signals
    signal(SIGSEGV, sighand);
    signal(SIGBUS, sighand);
    signal(SIGFPE, sighand);
    signal(SIGILL, sighand);
    signal(SIGABRT, sighand);
    signal(SIGTRAP, sighand);
    signal(SIGQUIT, sighand);

    CogServer& cogserve = cogserver();

    // Load modules specified in config
    cogserve.loadModules();

    // Enable the network server and run the server's main loop.
    if (0 < console_port)
        cogserve.enableNetworkServer(console_port);
    if (0 < webserver_port)
        cogserve.enableWebServer(webserver_port);
    if (0 < mcp_port)
        cogserve.enableMCPServer(mcp_port);
    cogserve.serverLoop();
    exit(0);
}

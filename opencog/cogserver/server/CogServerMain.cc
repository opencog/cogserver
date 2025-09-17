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

#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>

#include <opencog/cogserver/server/CogServer.h>
#include <opencog/cogserver/version.h>

using namespace opencog;

static void usage(const char* progname)
{
    std::cerr << "Usage: " << progname
        << " [-p <console port>] [-w <webserver port>] [-m <mcp port>] [-v] [-DOPTION=\"VALUE\"]\n"
        << "  -v    Print version and exit\n"
        << "\n"
        << "Supported options and default values:\n"
        << "SERVER_PORT = 17001\n"
        << "WEB_PORT = 18080\n"
        << "MCP_PORT = 18888\n"
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

    static const char *optString = "p:w:m:D:hv";
    std::vector<std::pair<std::string, std::string>> configPairs;
    std::string progname = argv[0];

    // parse command line
    while (true) {
        int c = getopt (argc, argv, optString);
        /* Detect end of options */
        if (c == -1) {
            break;
        } else if (c == 'D') {
            // override all previous options, e.g.
            // -DLOG_TO_STDOUT=TRUE
            std::string text = optarg;

            // Find the position of '=' separator
            size_t equalPos = text.find('=');

            if (equalPos == std::string::npos) {
                // No '=' found, option has no value
                std::cerr << "No value given for option "
                          << text << std::endl;
                configPairs.push_back({text, ""});
            } else {
                // Split at '=' position
                std::string optionName = text.substr(0, equalPos);
                std::string value = text.substr(equalPos + 1);
                configPairs.push_back({optionName, value});
            }
        } else if (c == 'p') {
            console_port = atoi(optarg);
        } else if (c == 'w') {
            webserver_port = atoi(optarg);
        } else if (c == 'm') {
            mcp_port = atoi(optarg);
        } else if (c == 'v') {
            std::cout << "CogServer version " << COGSERVER_VERSION_STRING << std::endl;
            exit(0);
        } else {
            // unknown option (or help)
            usage(progname.c_str());
            if (c == 'h')
                exit(0);
            else
                exit(1);
        }

    }

    // Copy command-line options to global cache.
    // This is ... deprecated, and should be removed. Later.
    for (const auto& optionPair : configPairs) {
        const std::string& opt = optionPair.first;
        const std::string& val = optionPair.second;
        // std::cerr << opt << " = " << val << std::endl;
        config().set(opt, val);

        if (0 == opt.compare("LOG_LEVEL"))
            logger().set_level(val);
        if (0 == opt.compare("LOG_FILE"))
            logger().set_filename(val);
        if (0 == opt.compare("LOG_TO_STDOUT"))
        {
            if (not ('f' == val[0] or 'F' == val[0] or '0' == val[0]))
                logger().set_print_to_stdout_flag(true);
        }
        if (0 == opt.compare("SERVER_PORT"))
            console_port = atoi(val.c_str());
        if (0 == opt.compare("WEB_PORT"))
            webserver_port = atoi(val.c_str());
        if (0 == opt.compare("MCP_PORT"))
            mcp_port = atoi(val.c_str());
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
    try
    {
        if (0 < console_port)
            cogserve.enableNetworkServer(console_port);
        if (0 < webserver_port)
            cogserve.enableWebServer(webserver_port);
        if (0 < mcp_port)
            cogserve.enableMCPServer(mcp_port);
    }
    catch (const std::exception& ex)
    {
        fprintf(stderr, "Error exit: %s\n", ex.what());
        exit(-1);
    }
    cogserve.serverLoop();
    exit(0);
}

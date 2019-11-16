/*
 * opencog/cogserver/server/RequestClassInfo.h
 *
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Gustavo Gama <gama@vettalabs.com>,
 * Simple-API implementation by Linas Vepstas <linasvepstas@gmail.com>
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * The Cogserver Request system is deprecated; users are encouraged to
 * explore writing guile (scheme) or python modules instead.
 */


#ifndef _OPENCOG_REQUEST_CLASS_INFO_H
#define _OPENCOG_REQUEST_CLASS_INFO_H

#include <string>

namespace opencog
{

/**
 * This struct defines the extended set of attributes used by opencog requests.
 * The current set of attributes are:
 *     id:          the name of the request
 *     description: a short description of what the request does
 *     help:        an extended description of the request, listing multiple
 *                  usage patterns and parameters
 */
struct RequestClassInfo : public ClassInfo
{
    std::string description;
    std::string help;
    bool is_shell;
    /** Whether default shell should be hidden from help */
    bool hidden;

    RequestClassInfo() : is_shell(false), hidden(false) {};
    RequestClassInfo(const char* i, const char *d, const char* h,
            bool s = false, bool hide = false)
        : ClassInfo(i), description(d), help(h), is_shell(s), hidden(hide) {};
    RequestClassInfo(const std::string& i, 
                     const std::string& d,
                     const std::string& h, 
                     bool s = false,
                     bool hide = false)
        : ClassInfo(i), description(d), help(h), is_shell(s), hidden(hide) {};
};


} // namespace 

#endif // _OPENCOG_REQUEST_CLASS_INFO_H

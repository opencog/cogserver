/*
 * opencog/cogserver/server/PageServer.h
 *
 * Copyright (C) 2025 BrainyBlaze Dynamics Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Serves static web pages from /usr/local/share/cogserver
 */

#ifndef _OPENCOG_PAGE_SERVER_H
#define _OPENCOG_PAGE_SERVER_H

#include <string>

namespace opencog
{

/**
 * PageServer - Serves static content from /usr/local/share/cogserver
 *
 * This class handles serving static HTML, CSS, JS, and favicon.ico files
 * from the cogserver's shared data directory.
 */
class PageServer
{
private:
    static const std::string base_path;

    /**
     * Get the MIME type for a file based on its extension
     */
    static std::string getMimeType(const std::string& filename);

    /**
     * Read a file from disk
     */
    static std::string readFile(const std::string& filepath);

    /**
     * Check if a file path is safe (no directory traversal)
     */
    static bool isSafePath(const std::string& path);

public:
    /**
     * Serve a static file if it exists, or return a 404 response
     *
     * @param url The requested URL path
     * @return Complete HTTP response including headers and body
     */
    static std::string serve(const std::string& url);

    /**
     * Generate a 404 Not Found response
     */
    static std::string notFound(const std::string& url);
};

} // namespace opencog

#endif // _OPENCOG_PAGE_SERVER_H
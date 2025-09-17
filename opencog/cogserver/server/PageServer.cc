/*
 * opencog/cogserver/server/PageServer.cc
 *
 * Copyright (C) 2025 BrainyBlaze Dynamics Inc.
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Implementation of static page server
 */

#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <opencog/util/Logger.h>
#include <opencog/cogserver/server/PageServer.h>

using namespace opencog;

const std::string PageServer::base_path = PROJECT_INSTALL_PREFIX "/share/cogserver";

std::string PageServer::getMimeType(const std::string& filename)
{
    size_t dot_pos = filename.rfind('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }

    std::string ext = filename.substr(dot_pos);

    if (ext == ".html" || ext == ".htm") {
        return "text/html";
    } else if (ext == ".css") {
        return "text/css";
    } else if (ext == ".js") {
        return "application/javascript";
    } else if (ext == ".ico") {
        return "image/x-icon";
    }

    return "application/octet-stream";
}

std::string PageServer::readFile(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return "";
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

bool PageServer::isSafePath(const std::string& path)
{
    // Check for directory traversal attempts
    if (path.find("..") != std::string::npos) {
        return false;
    }

    // Check for absolute paths
    if (!path.empty() && path[0] == '/') {
        // Allow paths starting with / but we'll strip it
        return true;
    }

    return true;
}

std::string PageServer::serve(const std::string& url)
{
    // Strip query string if present
    std::string path = url;
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path = path.substr(0, query_pos);
    }

    // Security check
    if (!isSafePath(path)) {
        logger().warn("[PageServer] Unsafe path requested: %s", path.c_str());
        return notFound(url);
    }

    // Build the full file path
    std::string filepath = base_path;

    // Handle root request
    if (path == "/" || path.empty()) {
        filepath += "/index.html";
    } else if (path[0] == '/') {
        filepath += path;
    } else {
        filepath += "/" + path;
    }

    // Check if file exists
    struct stat file_stat;
    if (stat(filepath.c_str(), &file_stat) != 0) {
        // File doesn't exist
        logger().debug("[PageServer] File not found: %s", filepath.c_str());
        return notFound(url);
    }

    // Make sure it's a regular file, not a directory
    if (!S_ISREG(file_stat.st_mode)) {
        // If it's a directory, try index.html
        if (S_ISDIR(file_stat.st_mode)) {
            filepath += "/index.html";
            if (stat(filepath.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
                logger().debug("[PageServer] Directory without index.html: %s", url.c_str());
                return notFound(url);
            }
        } else {
            logger().debug("[PageServer] Not a regular file: %s", filepath.c_str());
            return notFound(url);
        }
    }

    // Read the file
    std::string content = readFile(filepath);
    if (content.empty()) {
        logger().warn("[PageServer] Could not read file: %s", filepath.c_str());
        return notFound(url);
    }

    // Build HTTP response
    std::string mime_type = getMimeType(filepath);

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Server: CogServer\r\n";
    response << "Content-Type: " << mime_type << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "\r\n";
    response << content;

    logger().debug("[PageServer] Served %s (%s, %zu bytes)",
                  url.c_str(), mime_type.c_str(), content.length());

    return response.str();
}

std::string PageServer::notFound(const std::string& url)
{
    std::string body =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head><meta charset=\"UTF-8\"></head>\n"
        "<body><h1>404 Not Found</h1>\n"
        "The Cogserver doesn't know about " + url + "\n"
        "<p>The <a href=\"/stats\">stats page is here</a>.\n"
        "<p>The <a href=\"/websockets/json-test.html\">JSON test page is here</a>.\n"
        "<p>The <a href=\"/websockets/demo.html\">websockets demo is here</a>.\n"
        "<p>The <a href=\"/visualizer/\">visualizer is here</a>.\n"
        "</body></html>\n";

    std::ostringstream response;
    response << "HTTP/1.1 404 Not Found\r\n";
    response << "Server: CogServer\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "\r\n";
    response << body;

    return response.str();
}

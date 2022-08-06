/*
 * opencog/network/WebSocket.cc
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>
#include <sys/prctl.h>

#include <opencog/util/Logger.h>

#include <opencog/network/WebSocket.h>

using namespace opencog;

WebSocket::WebSocket(void)
{
}

WebSocket::~WebSocket()
{
    logger().debug("[WebSocket] destructor");
}

// ==================================================================
// Connection states for debugging.
static char IWAIT[6] = "iwait";
static char RUN  [6] = " run ";
static char CLOSE[6] = "close";

void WebSocket::handle_connection(void)
{
    prctl(PR_SET_NAME, "cogserv:connect", 0, 0, 0);
    _tid = gettid();
    _pth = pthread_self();
    logger().debug("WebSocket::handle_connection()");
    OnConnection();
    boost::asio::streambuf b;
    while (true)
    {
        try
        {
            _status = IWAIT;
            boost::asio::read_until(*_socket, b, match_eol_or_escape);
            std::istream is(&b);
            std::string line;
            std::getline(is, line);
            if (not line.empty() and line[line.length()-1] == '\r') {
                line.erase(line.end()-1);
            }

            _last_activity = time(nullptr);
            _line_count++;
            total_line_count++;
            _status = RUN;
            OnLine(line);
        }
        catch (const boost::system::system_error& e)
        {
            if (e.code() == boost::asio::error::eof) {
                break;
            } else if (e.code() == boost::asio::error::connection_reset) {
                break;
            } else if (e.code() == boost::asio::error::not_connected) {
                break;
            } else {
                logger().error("WebSocket::handle_connection(): Error reading data. Message: %s", e.what());
            }
        }
    }

    _last_activity = time(nullptr);
    _status = CLOSE;

    logger().debug("WebSocket::exiting handle_connection()");

    // In the standard scenario, WebSocket inherits from this, and
    // so deleting this will cause the WebSocket dtor to run. This
    // will, in turn, try to delete the shell, which will typically
    // stall until the current evaluation is done. If the current
    // evaluation is an infinite loop, then it will hang forever, and
    // gdb will show a stack trace stuck in GenericShell::while_not_done()
    // This is perfectly normal, and nothing can be done about it; we
    // can't kill it without hurting users who launch long-running but
    // finite commands via netcat. Nor can we magically unwind all the
    // C++ state and stacks, to leave only some very naked evaluator
    // running. The hang here, in the dtor, while_not_done(), really
    // must be thought of as the normal sync point for completion.
    delete this;
}

// ==================================================================

std::string WebSocket::connection_header(void)
{
    return ServerSocket::connection_header() + " U SHEL QZ E PENDG";
}

std::string WebSocket::connection_stats(void)
{
    char buf[40];
    snprintf(buf, 40, " %1d ", get_use_count());

    std::string rc = ServerSocket::connection_stats() + buf;

    rc += "webs           ";

    return rc;
}

// ==================================================================

/*
 * opencog/network/ConsoleSocket.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Written by Andre Senna <senna@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <string>
#include <sys/prctl.h>

#include <opencog/util/Config.h>
#include <opencog/util/Logger.h>
#include <opencog/util/misc.h>

#include <opencog/network/ConsoleSocket.h>

using namespace opencog;

ConsoleSocket::ConsoleSocket(void)
{
    _shell = nullptr;
    _use_count = 0;
}

ConsoleSocket::~ConsoleSocket()
{
    logger().debug("[ConsoleSocket] destructor");

    // We need the use-count and the condition variables because
    // the design of boost:asio is broken. Basically, the boost::asio
    // code calls this destructor for ConsoleSocket while there are
    // still requests outstanding in another thread.  We have to stall
    // the destructor until all the in-flight requests are complete;
    // we use the condition variable to do this.
    //
    // Some details: basically, the remote end of the socket "fires and
    // forgets" a bunch of commands, and then closes the socket before
    // these requests have completed.  boost:asio notices that the
    // remote socket has closed, and so decides its a good day to call
    // destructors. But of course, its not ... because the requests
    // still have to be handled.
    std::unique_lock<std::mutex> lck(_in_use_mtx);
    while (_use_count) _in_use_cv.wait(lck);
    lck.unlock();

    // If there's a shell, kill it.
    if (_shell) delete _shell;

    logger().debug("[ConsoleSocket] destructor finished");
}

void ConsoleSocket::SetShell(GenericShell *g)
{
    _shell = g;

	// Push out a new prompt, when the shell closes.
	if (nullptr == g) OnLine("");
}

// ==================================================================

typedef boost::asio::buffers_iterator<
    boost::asio::streambuf::const_buffers_type> bitter;

// Some random RFC 854 characters
#define IAC 0xff  // Telnet Interpret As Command
#define IP 0xf4   // Telnet IP Interrupt Process
#define AO 0xf5   // Telnet AO Abort Output
#define EL 0xf8   // Telnet EL Erase Line
#define WILL 0xfb // Telnet WILL
#define DO 0xfd   // Telnet DO
#define TIMING_MARK 0x6 // Telnet RFC 860 timing mark
#define TRANSMIT_BINARY 0x0 // Telnet RFC 856 8-bit-clean
#define CHARSET 0x2A // Telnet RFC 2066


// Goal: if the user types in a ctrl-C or a ctrl-D, we want to
// react immediately to this. A ctrl-D is just the ascii char 0x4
// while the ctrl-C is wrapped in a telnet "interpret as command"
// IAC byte secquence.  Basically, we want to forward all IAC
// sequences immediately, as well as the ctrl-D.
//
// Currently not implemented, but could be: support for the arrow
// keys, which generate the sequence 0x1b 0x5c A B C or D.
//
std::pair<bitter, bool>
match_eol_or_escape(bitter begin, bitter end)
{
    bool telnet_mode = false;
    bitter i = begin;
    while (i != end)
    {
        unsigned char c = *i++;
        if (IAC == c) telnet_mode = true;
        if (('\n' == c) ||
            (0x04 == c) || // ASCII EOT End of Transmission (ctrl-D)
            (telnet_mode && (c <= 0xf0)))
        {
            return std::make_pair(i, true);
        }
    }
    return std::make_pair(i, false);
}

// ==================================================================
// Connection states for debugging.
static char IWAIT[6] = "iwait";
static char RUN  [6] = " run ";
static char CLOSE[6] = "close";

void ConsoleSocket::handle_connection(void)
{
    prctl(PR_SET_NAME, "cogserv:connect", 0, 0, 0);
    _tid = gettid();
    _pth = pthread_self();
    logger().debug("ServerSocket::handle_connection()");
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
                logger().error("ServerSocket::handle_connection(): Error reading data. Message: %s", e.what());
            }
        }
    }

    _last_activity = time(nullptr);
    _status = CLOSE;

    // If the data sent to us is not new-line terminated, then
    // there may still be some bytes sitting in the buffer. Get
    // them and forward them on.  These are typically scheme
    // strings issued from netcat, that simply did not have
    // newlines at the end.
    std::istream is(&b);
    std::string line;
    std::getline(is, line);
    if (not line.empty() and line[line.length()-1] == '\r') {
        line.erase(line.end()-1);
    }
    if (not line.empty())
        OnLine(line);

    logger().debug("ServerSocket::exiting handle_connection()");

    // In the standard scenario, ConsoleSocket inherits from this, and
    // so deleting this will cause the ConsoleSocket dtor to run. This
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

std::string ConsoleSocket::connection_header(void)
{
    return ServerSocket::connection_header() + " U SHEL QZ E PENDG";
}

std::string ConsoleSocket::connection_stats(void)
{
    char buf[40];
    snprintf(buf, 40, " %1d ", get_use_count());

    std::string rc = ServerSocket::connection_stats() + buf;

    if (_shell)
    {
        rc += _shell->_name;
        snprintf(buf, 40, " %2zd %c %5zd",
            _shell->queued(), _shell->eval_done()?'F':'T',
            _shell->pending());
        rc += buf;
    }
    else rc += "cogs           ";

    return rc;
}

// ==================================================================

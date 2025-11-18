/*
 * opencog/network/ServerSocket.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2010 Linas Vepstas <linasvepstas@gmail.com>
 * Written by Welter Luigi <welter@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <time.h>
#include <mutex>
#include <set>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/network/ServerSocket.h>
#include <opencog/network/SocketManager.h>

using namespace opencog;

// ==================================================================
// Infrastrucure for printing connection stats
//
char ServerSocket::START[6] = "start";
char ServerSocket::BLOCK[6] = "block";
char ServerSocket::IWAIT[6] = "iwait";
char ServerSocket::BAR[6]   = "-bar-";
char ServerSocket::DTOR[6]  = "dtor ";
char ServerSocket::QUING[6] = "quing";
char ServerSocket::CLOSE[6] = "close";
char ServerSocket::DOWN[6]  = "down ";

std::string ServerSocket::connection_header(void)
{
    return "OPEN-DATE        THREAD  STATE NLINE  LAST-ACTIVITY  K";
}

std::string ServerSocket::connection_stats(void)
{
    struct tm tm;

    // Start date
    char sbuff[20];
    gmtime_r(&_start_time, &tm);
    strftime(sbuff, 20, "%d %b %H:%M:%S", &tm);

    // Most recent activity
    char abuff[20];
    gmtime_r(&_last_activity, &tm);
    strftime(abuff, 20, "%d %b %H:%M:%S", &tm);

    // Thread ID as shown by `ps -eLf`
    char bf[132];
    snprintf(bf, 132, "%s %8d %s %5zd %s %c",
        sbuff, _tid, _status, _line_count, abuff,
        _do_frame_io?'W':
            (_is_http_socket?'H':
                (_is_mcp_socket?'M':'T')));

    return bf;
}

// ==================================================================

std::atomic_size_t ServerSocket::total_line_count(0);

ServerSocket::ServerSocket(SocketManager* mgr) :
    _socket(nullptr),
    _socket_manager(mgr),
    _got_first_line(false),
    _got_http_header(false),
    _do_frame_io(false),
    _is_http_socket(false),
    _got_websock_header(false),
    _is_mcp_socket(false),
    _keep_alive(false),
    _in_barrier(false),
    _content_length(0),
    _host_header("")
{
    _start_time = time(nullptr);
    _last_activity = _start_time;
    _tid = 0;
    _pth = 0;
    _status = BLOCK;
    _line_count = 0;

    // Register with socket manager
    _socket_manager->add_sock(this);

    // Block here, if there are too many concurrently-open sockets.
    _socket_manager->wait_available_slot();
    _status = START;
}

ServerSocket::~ServerSocket()
{
    _status = DTOR;
    logger().debug("ServerSocket::~ServerSocket()");

    Exit();

    // An attempt to delete an asio socket, after being stopped with
    // `asio::io_service::stop()` will result in a crash, deep
    // inside asio. Failing to delete is also obviously a memleak,
    // but for now, well accept a memleak in exchange for stability.
    // See notes in the body of the Exit() method below (circa line 322).
    if (not _socket_manager->is_network_gone())
        delete _socket;

    _socket = nullptr;

    // Unregister from socket manager
    _socket_manager->rem_sock(this);

    // If anyone is waiting for a socket, let them know that
    // we've freed one up.
    _socket_manager->release_slot();
}

// ==================================================================

void ServerSocket::Send(const std::string& cmd)
{
    size_t cmdsize = cmd.size();

    // Avoid spurious zero-length packets. They have no meaning.
    if (0 == cmdsize) return;

    // Avoid lonely newlines. The various shells return these for
    // no particular reason.  Due to old confusions about line
    // discipline. Just avoid them. The only place this might
    // matter would be python, and the "obvious" solution is to
    // use two newlines, or a crlf.
    if (1 == cmdsize and '\n' == cmd[0]) return;

    if (not _do_frame_io)
    {
        Send(asio::const_buffer(cmd.c_str(), cmdsize));
        return;
    }

    // If we are here, we have to perform websockets framing.
    send_websocket(cmd);
}

void ServerSocket::Send(const asio::const_buffer& buf)
{
    OC_ASSERT(_socket, "Use of socket after it's been closed!\n");

    std::error_code error;
    asio::write(*_socket, buf,
                       asio::transfer_all(), error);

    // The most likely cause of an error is that the remote side has
    // closed the socket, even though we still had stuff to send.
    // I believe this is a ENOTCON errno, maybe others as well.
    // (for example, ECONNRESET `Connection reset by peer`)
    // Don't log these harmless errors.
    // Do log true failures.
    if (error and
        error.value() != asio::error::not_connected and
        error.value() != asio::error::broken_pipe and
        error.value() != asio::error::bad_descriptor and
        error.value() != asio::error::connection_reset)
        logger().warn("ServerSocket::Send(): %s on thread 0x%x\n",
             error.message().c_str(), pthread_self());
}

// As far as I can tell, asio is not actually thread-safe,
// in particular, when closing and destroying sockets.  This strikes
// me as incredibly stupid -- a first-class reason to not use asio.
// But whatever.  Hack around this for now.
static std::mutex _asio_crash;

// This is called in a different thread than the thread that is running
// the handle_connection() method. It's purpose in life is to terminate
// the connection -- it does so by closing the socket. Sometime later,
// the handle_connection() method notices that it's closed, and exits
// it's loop, thus ending the thread that its running in.
void ServerSocket::Exit()
{
    std::lock_guard<std::mutex> lock(_asio_crash);
    logger().debug("ServerSocket::Exit()");
    try
    {
        _socket->shutdown(asio::ip::tcp::socket::shutdown_both);

        // OK, so there is some asio bug here. This line of code
        // crashes, and I can't figure out how to make it not crash.
        // So, if we start a cogserver, telnet into it, stop the
        // cogserver, then exit telnet, it will crash deep inside of
        // asio (in the `close()` below.) I think it crashes because
        // asio is accessing freed memory. That is, by this point,
        // we have called `NetworkServer::stop()` which calls
        // `asio::io_service::stop()` which probably frees
        // something. Of course, we only wanted to stop asio from
        // listening, and not to stop it from servicing sockets. So
        // anyway, it frees stuff, and then does a use-after-free.
        //
        // Why do I think this? Because, on rare occasions, it does not
        // crash in the `close()` below. It crashes later, in `malloc()`
        // with a corrupted free list. Which tells me asio is doing
        // use-after-free.
        //
        // ASIO seems awfully buggy to me ... its forced us into
        // this stunningly complex design, and ... I don't know how to
        // (easily) fix it.
        //
        // If we don't close the socket, then it crashes in the same
        // place in the destructor. If we don't call the destructor,
        // then memory leaks.
        //
        // The long-term solution is to rewrite this code to not use
        // asio. But that is just a bit more than a weekend project.
        if (not _socket_manager->is_network_gone())
            _socket->close();
    }
    catch (const std::system_error& e)
    {
        if (e.code() != asio::error::not_connected and
            e.code() != asio::error::bad_descriptor)
        {
            logger().error("ServerSocket::Exit(): Error closing socket: %s", e.what());
        }
    }
    _status = DOWN;
}

// ==================================================================

void ServerSocket::set_connection(asio::ip::tcp::socket* sock)
{
    if (_socket) delete _socket;
    _socket = sock;
}

// ==================================================================

typedef asio::buffers_iterator<
    asio::streambuf::const_buffers_type> bitter;

// See RFC 854
#define IAC 0xff  // Telnet Interpret As Command

// Goal: if the user types in a ctrl-C or a ctrl-D, we want to
// react immediately to this. A ctrl-D is just the ascii char 0x4
// while the ctrl-C is wrapped in a telnet "interpret as command"
// IAC byte sequence.  Basically, we want to forward all IAC
// sequences immediately, as well as the ctrl-D.
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

/// Read a single newline-delimited line from the socket.
/// Return immediately if a ctrl-C or ctrl-D is found.
std::string ServerSocket::get_telnet_line(asio::streambuf& b)
{
    asio::read_until(*_socket, b, match_eol_or_escape);
    std::istream is(&b);
    std::string line;
    std::getline(is, line);
    return line;
}

/// Read _content_length bytes from the socket.
std::string ServerSocket::get_http_body(asio::streambuf& b)
{
    if (_content_length == 0) return "";

    // Need to read more data
    if (b.size() < _content_length)
    {
        size_t remaining = _content_length - b.size();
        asio::read(*_socket, b,
            asio::transfer_exactly(remaining));
    }

    std::string body;
    body.resize(_content_length);
    std::istream is(&b);
    is.read(&body[0], _content_length);
    return body;
}

// ==================================================================

// This method is called in a new thread, when a new network connection
// is made. It handles all socket reads for that socket.
void ServerSocket::handle_connection(void)
{
    prctl(PR_SET_NAME, "cogserv:connect", 0, 0, 0);
    _tid = gettid();
    _pth = pthread_self();
    logger().debug("ServerSocket::handle_connection()");

    // telnet sockets have no setup to do.
    if (not _is_http_socket)
        OnConnection();
    asio::streambuf b;
    while (true)
    {
        try
        {
            _status = IWAIT;
            std::string line;
            if (not _do_frame_io)
                line = get_telnet_line(b);
            else
                line = get_websocket_line();

            // Some local Linux D-Bus daemon desperately wants to
            // talk to us, sending us binary garbage of some kind.
            // Desperately ignore it.
            if (1 < line.size() and
                0x1 == line.c_str()[0] and 0x21 == line.c_str()[1]) break;

            // Strip off carriage returns. The line already stripped
            // newlines.
            if (not line.empty() and line[line.length()-1] == '\r') {
                line.erase(line.end()-1);
            }

            _last_activity = time(nullptr);
            _line_count++;
            total_line_count++;
            _status = QUING;

            // If its not an http sock, then the API is simple.
            if (not _is_http_socket)
                OnLine(line);
            else
            {
                // Bypass until we've received the full HTTP header.
                if (not _got_http_header)
                    HandshakeLine(line);
                if (_got_http_header)
                {
                    // Process the complete HTTP request
                    // If we're running websockets, then we're doing
                    // frame I/O and we've already got the line.
                    // Otherwise, we use http Content-Length.
                    if (_do_frame_io)
                        OnLine(line);
                    else
                    {
                        std::string http_body(get_http_body(b));
                        OnLine(http_body);

                        // Reset for next HTTP request.
                        _got_http_header = false;
                        _got_first_line = false;
                        _content_length = 0;
                    }
                }
            }
        }
        catch (const std::system_error& e)
        {
            if (e.code() == asio::error::eof) {
                // EOF received, but there may still be data in the socket
                // buffer that hasn't been read yet. Try to drain it before
                // breaking. This handles the case where a client sends
                // multiple commands and immediately closes (nc -q 0).
                try {
                    std::error_code ec;
                    size_t avail = _socket->available(ec);
                    if (not ec and 0 < avail) {
                        asio::read(*_socket, b, asio::transfer_exactly(avail), ec);
                        // If we successfully read more data, continue the loop
                        // to process it. Otherwise, break out.
                        if (not ec) continue;
                    }
                } catch (...) {}
                break;
            } else if (e.code() == asio::error::connection_reset) {
                break;
            } else if (e.code() == asio::error::not_connected) {
                break;
            } else if (e.code() == asio::error::bad_descriptor) {
                // We hit this during cogserver shutdown.
                break;
            } else {
                logger().error("ServerSocket::handle_connection(): Error reading data. Message: %s", e.what());
            }
        }
        catch (const SilentException& e)
        {
            break;
        }
    }

    _last_activity = time(nullptr);
    _status = CLOSE;

    // Perform cleanup at end, if in telnet mode.
    if (not _is_http_socket)
    {
        // If the data sent to us is not new-line terminated, then
        // there may still be some bytes sitting in the buffer. Get
        // them and forward them on.  These are typically scheme
        // strings issued from netcat, that simply did not have
        // newlines at the end. There may be multiple lines buffered,
        // so drain all of them.
        std::istream is(&b);
        std::string line;
        while (std::getline(is, line))
        {
            if (not line.empty() and line[line.length()-1] == '\r') {
                line.erase(line.end()-1);
            }
            if (not line.empty())
                OnLine(line);
        }
    }

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

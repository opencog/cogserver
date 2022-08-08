/*
 * opencog/network/ServerSocket.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2010 Linas Vepstas <linasvepstas@gmail.com>
 * Written by Welter Luigi <welter@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <sys/prctl.h>
#include <sys/types.h>
#include <time.h>
#include <mutex>
#include <set>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/network/ServerSocket.h>

using namespace opencog;

// ==================================================================
// Infrastrucure for printing connection stats
//
static char START[6] = "start";
static char BLOCK[6] = "block";
static char IWAIT[6] = "iwait";
static char DTOR [6] = "dtor ";
static char RUN  [6] = " run ";
static char CLOSE[6] = "close";
static char DOWN [6] = "down ";

static std::mutex _sock_lock;
static std::set<ServerSocket*> _sock_list;

static void add_sock(ServerSocket* ss)
{
    std::lock_guard<std::mutex> lock(_sock_lock);
    _sock_list.insert(ss);
}

static void rem_sock(ServerSocket* ss)
{
    std::lock_guard<std::mutex> lock(_sock_lock);
    _sock_list.erase(ss);
}

std::string ServerSocket::display_stats(void)
{
    // Hack(?) Send a half-ping, in an attempt to close
    // dead connections.
    half_ping();

    std::string rc;
    std::lock_guard<std::mutex> lock(_sock_lock);

    // Make a copy, and sort it.
    std::vector<ServerSocket*> sov;
    for (ServerSocket* ss : _sock_list)
        sov.push_back(ss);

    std::sort (sov.begin(), sov.end(),
        [](ServerSocket* sa, ServerSocket* sb) -> bool
        { return sa->_start_time == sb->_start_time ?
            sa->_tid < sb->_tid :
            sa->_start_time < sb->_start_time; });

    // Print the sorted list; use the first to print a header.
    bool hdr = false;
    for (ServerSocket* ss : sov)
    {
        if (not hdr)
        {
            rc += ss->connection_header() + "\n";
            hdr = true;
        }
        rc += ss->connection_stats() + "\n";
    }

    return rc;
}

// Send a single blank character to each socket.
// If the socket is only half-open, this should result
// in the socket closing fully.  If the socket is fully
// open, then the remote end will receive a blank space.
// Since we are running a UTF-8 character protocol, this
// should be harmless. Another possibility is to send
// hex 0x16 ASCII SYN synchronous idle. Will this confuse
// any users of the cogserver? I dunno.  Lets go for SYN.
// It's slightly cleaner.
//
// For websockets, this sends the pong frame, which has
// the same effect.
void ServerSocket::half_ping(void)
{
    // static const char buf[2] = " ";
    static const char buf[2] = {0x16, 0x0};

    std::lock_guard<std::mutex> lock(_sock_lock);
    time_t now = time(nullptr);

    for (ServerSocket* ss : _sock_list)
    {
        // If the socket is waiting on input, and has been idle
        // for more than ten seconds, then ping it to see if it
        // is still alive.
        if (ss->_status == IWAIT and
            now - ss->_last_activity > 10)
        {
            if (ss->_do_frame_io)
                ss->send_websocket_pong();
            else
                ss->Send(buf);
        }
    }
}

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
        _is_websocket?'W':'T');

    return bf;
}

// ==================================================================

/// Kill the indicated thread id.
// TODO: should use std::jthread, once c++20 is widely available.
bool ServerSocket::kill(pid_t tid)
{
    std::lock_guard<std::mutex> lock(_sock_lock);

    for (ServerSocket* ss : _sock_list)
    {
        if (tid == ss->_tid)
        {
            ss->Exit();
            // pthread_cancel(ss->_pth);
            return true;
        }
    }
    return false;
}

// ==================================================================

std::atomic_size_t ServerSocket::total_line_count(0);

// _max_open_sockets is the largest number of concurrently open
// sockets we will allow in the server. Currently set to 60.
// Note that each SchemeShell (actually, SchemeEval) will open
// another half-dozen pipes and what-not, so actually, the number
// of open files will increase by 4 or 6 or so for each network
// connection. With the default `ulimit -a` of 1024 open files,
// this should work OK (including open files for the logger, the
// databases, etc.).
//
// July 2019 - change to 10. When it is 60, it just thrashes like
// crazy, mostly because there are 60 threads thrashing in guile
// on some lock. And that's pretty pointless...
unsigned int ServerSocket::_max_open_sockets = 10;
volatile unsigned int ServerSocket::_num_open_sockets = 0;
std::mutex ServerSocket::_max_mtx;
std::condition_variable ServerSocket::_max_cv;
size_t ServerSocket::_num_open_stalls = 0;

ServerSocket::ServerSocket(void) :
    _socket(nullptr),
    _got_first_line(false),
    _got_http_header(false),
    _do_frame_io(false),
    _is_websocket(false),
    _got_websock_header(false)
{
    _start_time = time(nullptr);
    _last_activity = _start_time;
    _tid = 0;
    _pth = 0;
    _status = BLOCK;
    _line_count = 0;
    add_sock(this);

    // Block here, if there are too many concurrently-open sockets.
    std::unique_lock<std::mutex> lck(_max_mtx);
    _num_open_sockets++;

    // If we are just below the max limit, send a half-ping in an
    // attempt to force any half-open connections to close.
    if (_max_open_sockets <= _num_open_sockets)
        half_ping();

    // Report how often we stall because we hit the max.
    if (_max_open_sockets < _num_open_sockets)
        _num_open_stalls ++;

    while (_max_open_sockets < _num_open_sockets) _max_cv.wait(lck);
    _status = START;
}

ServerSocket::~ServerSocket()
{
    _status = DTOR;
    logger().debug("ServerSocket::~ServerSocket()");

    Exit();
    delete _socket;
    _socket = nullptr;
    rem_sock(this);

    // If anyone is waiting for a socket, let them know that
    // we've freed one up.
    std::unique_lock<std::mutex> mxlck(_max_mtx);
    _num_open_sockets--;
    _max_cv.notify_all();
    mxlck.unlock();
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
        Send(boost::asio::const_buffer(cmd.c_str(), cmdsize));
        return;
    }

    // If we are here, we have to perform websockets framing.
    // Send only one packet, and indicate it's length.
    size_t paylen = cmdsize;
    char header[10];
    header[0] = 0x81;
    if (paylen < 126)
    {
        header[1] = (char) paylen;
        Send(boost::asio::const_buffer(header, 2));
    }
    else if (paylen < 65536)
    {
        header[1] = 126;
        header[2] = (paylen >> 8) & 0xff;
        header[3] = paylen & 0xff;
        Send(boost::asio::const_buffer(header, 4));
    }
    else
    {
        header[1] = 127;
        header[2] = (paylen >> 56) & 0xff;
        header[3] = (paylen >> 48) & 0xff;
        header[4] = (paylen >> 40) & 0xff;
        header[5] = (paylen >> 32) & 0xff;
        header[6] = (paylen >> 24) & 0xff;
        header[7] = (paylen >> 16) & 0xff;
        header[8] = (paylen >> 8) & 0xff;
        header[9] = paylen & 0xff;
        Send(boost::asio::const_buffer(header, 10));
    }

    // Send the actual data.
    Send(boost::asio::const_buffer(cmd.c_str(), cmdsize));
}

void ServerSocket::Send(const boost::asio::const_buffer& buf)
{
    OC_ASSERT(_socket, "Use of socket after it's been closed!\n");

    boost::system::error_code error;
    boost::asio::write(*_socket, buf,
                       boost::asio::transfer_all(), error);

    // The most likely cause of an error is that the remote side has
    // closed the socket, even though we still had stuff to send.
    // I beleive this is a ENOTCON errno, maybe others as well.
    // (for example, ECONNRESET `Connection reset by peer`)
    // Don't log these harmless errors.
    // Do log true failures.
    if (error.value() != boost::system::errc::success and
        error.value() != boost::asio::error::not_connected and
        error.value() != boost::asio::error::broken_pipe and
        error.value() != boost::asio::error::bad_descriptor and
        error.value() != boost::asio::error::connection_reset)
        logger().warn("ServerSocket::Send(): %s on thread 0x%x\n",
             error.message().c_str(), pthread_self());
}

// As far as I can tell, boost::asio is not actually thread-safe,
// in particular, when closing and destroying sockets.  This strikes
// me as incredibly stupid -- a first-class reason to not use boost.
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
        _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        // OK, so there is some boost bug here. This line of code
        // crashes, and I can't figure out how to make it not crash.
        // So, if we start a cogserver, telnet into it, stop the
        // cogserver, then exit telnet, it will crash deep inside of
        // boost (in the `close()` below.) I think it crashes because
        // boost is accessing freed memory. That is, by this point,
        // we have called `NetworkServer::stop()` which calls
        // `boost::asio::io_service::stop()` which probably frees
        // something. Of course, we only wanted to stop boost from
        // listening, and not to stop it from servicing sockets. So
        // anyway, it frees stuff, and then does a use-after-free.
        //
        // Why do I think this? Because, on rare occasions, it does not
        // crash in the `close()` below. It crashes later, in `malloc()`
        // with a corrupted free list. Which tells me boost is doing
        // use-after-free.
        //
        // Boost ASIO seems awfully buggy to me ... its forced us into
        // this stunningly complex design, and ... I don't know how to
        // (easily) fix it.
        //
        // If we don't close the socket, then it crashes in the same
        // place in the destructor. If we don't call the destructor,
        // then memory leaks.
        //
        // The long-term solution is to rewrite this code to not use
        // asio. But that is just a bit more than a weekend project.
        _socket->close();
    }
    catch (const boost::system::system_error& e)
    {
        if (e.code() != boost::asio::error::not_connected and
            e.code() != boost::asio::error::bad_descriptor)
        {
            logger().error("ServerSocket::Exit(): Error closing socket: %s", e.what());
        }
    }
    _status = DOWN;
}

// ==================================================================

void ServerSocket::set_connection(boost::asio::ip::tcp::socket* sock)
{
    if (_socket) delete _socket;
    _socket = sock;
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

/// Read a single newline-delimited line from the socket.
/// Return immediately if a ctrl-C or ctrl-D is found.
std::string ServerSocket::get_telnet_line(boost::asio::streambuf& b)
{
    boost::asio::read_until(*_socket, b, match_eol_or_escape);
    std::istream is(&b);
    std::string line;
    std::getline(is, line);
    return line;
}

// ==================================================================

// Ths method is called in a new thread, when a new network connection is
// made. It handles all socket reads for that socket.
void ServerSocket::handle_connection(void)
{
    prctl(PR_SET_NAME, "cogserv:connect", 0, 0, 0);
    _tid = gettid();
    _pth = pthread_self();
    logger().debug("ServerSocket::handle_connection()");

    // telent sockets have no setup to do.
    if (not _is_websocket)
        OnConnection();
    boost::asio::streambuf b;
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

            // Strip off carriage returns. The line already stripped
            // newlines.
            if (not line.empty() and line[line.length()-1] == '\r') {
                line.erase(line.end()-1);
            }

            _last_activity = time(nullptr);
            _line_count++;
            total_line_count++;
            _status = RUN;

				// Bypass until we've got the WebSocket fully open.
				if (_is_websocket and not _do_frame_io)
					HandshakeLine(line);
				else
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
        catch (const SilentException& e)
        {
            break;
        }
    }

    _last_activity = time(nullptr);
    _status = CLOSE;

    // Perform cleanup at end, if in telnet mode.
    if (not _is_websocket)
    {
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

/*
 * opencog/network/SocketManager.cc
 *
 * Copyright (C) 2025 Linas Vepstas
 * Copyright (C) 2002-2007 Novamente LLC
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <vector>

#include <opencog/util/Logger.h>
#include <opencog/util/oc_assert.h>
#include <opencog/network/SocketManager.h>
#include <opencog/network/ServerSocket.h>
#include <opencog/network/ConsoleSocket.h>
#include <opencog/network/GenericShell.h>

using namespace opencog;

SocketManager::SocketManager()
	: _max_open_sockets(0),
	  _num_open_sockets(0),
	  _num_open_stalls(0),
	  _barrier_active(false),
	  _network_gone(false)
{
	// Set max open sockets to number of hardware CPUs
	unsigned int hwlim = std::thread::hardware_concurrency();
	if (0 == hwlim) hwlim = 32;
	_max_open_sockets = hwlim;

	// Revise number of open files upwards, if needed.
	// A typical operating condition is that each network
	// connection to the cogserver results in 6-8 other open
	// file descriptors. We'll be paranoid, and set this to 16x.
	rlim_t wanted = 16 * _max_open_sockets;

	// The only problem here is that we don't have CAP_SYS_RESOURCE
	// and only the shell user can set this. So in reality, it is
	// already the case that rlim.rlim_cur == rlim.rlim_max and
	// we can't do anything to change this. So instead, just print
	// a warning. Oh well.
	struct rlimit rlim;
	int rc = getrlimit(RLIMIT_NOFILE, &rlim);
	if (0 == rc and rlim.rlim_cur < wanted)
	{
		if (rlim.rlim_max < wanted)
		{
			fprintf(stderr,
"Warning: Cogserver: you may want to increase the max open files\n"
"Use the `ulimit -a` command to view this, and `ulimit -n` to set it.\n"
"Recommend setting this to %lu open file descriptors.\n", wanted);
			logger().warn(
"Warning: Cogserver: you may want to increase the max open files\n"
"Use the `ulimit -a` command to view this, and `ulimit -n` to set it.\n"
"Recommend setting this to %lu open file descriptors.\n", wanted);
			wanted = rlim.rlim_max;
		}
		rlim.rlim_cur = wanted;
		setrlimit(RLIMIT_NOFILE, &rlim);
	}
}

SocketManager::~SocketManager()
{
}

void SocketManager::add_sock(ServerSocket* ss)
{
	std::lock_guard<std::mutex> lock(_sock_lock);
	_sock_list.insert(ss);
}

void SocketManager::rem_sock(ServerSocket* ss)
{
	std::lock_guard<std::mutex> lock(_sock_lock);
	_sock_list.erase(ss);
}

void SocketManager::wait_available_slot()
{
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
}

void SocketManager::release_slot()
{
	std::unique_lock<std::mutex> mxlck(_max_mtx);
	_num_open_sockets--;
	_max_cv.notify_all();
}

std::string SocketManager::display_stats_full(const char* title, time_t start_time, int nlines)
{
	struct tm tm;
	char sbuf[40];
	gmtime_r(&start_time, &tm);
	strftime(sbuf, 40, "%d %b %H:%M:%S %Y", &tm);

	char nbuf[40];
	time_t now = time(nullptr);
	gmtime_r(&now, &tm);
	strftime(nbuf, 40, "%d %b %H:%M:%S %Y", &tm);

	// Current max_open_sockets is 60 which requires a terminal
	// size of 66x80 to display correctly. So reserve a reasonable
	// string size.
	std::string rc;
	rc.reserve(4000);

	rc = "----- OpenCog ";
	rc += title;
	rc += ": type help or ^C to exit\n";
	rc += nbuf;
	rc += " UTC ---- up-since: ";
	rc += sbuf;
	rc += "\n";

	// Count open file descs
	int nfd = 0;
	for (int j=0; j<4096; j++) {
		int fd = dup(j);
		if (fd < 0) continue;
		close(fd);
		nfd++;
	}

	char buff[180];
	snprintf(buff, sizeof(buff),
		"max-open-socks: %d   cur-open-socks: %d   num-open-fds: %d  stalls: %zd\n",
		_max_open_sockets, _num_open_sockets, nfd, _num_open_stalls);
	rc += buff;

	clock_t clk = clock();
	int sec = clk / CLOCKS_PER_SEC;
	clock_t rem = clk - sec * CLOCKS_PER_SEC;
	int msec = (1000 * rem) / CLOCKS_PER_SEC;

	struct rusage rus;
	getrusage(RUSAGE_SELF, &rus);

	snprintf(buff, sizeof(buff),
		"cpu: %d.%03d secs  user: %ld.%03ld  sys: %ld.%03ld     tot-lines: %lu\n",
		sec, msec,
		rus.ru_utime.tv_sec, rus.ru_utime.tv_usec / 1000,
		rus.ru_stime.tv_sec, rus.ru_stime.tv_usec / 1000,
		ServerSocket::total_line_count.load());
	rc += buff;

	snprintf(buff, sizeof(buff),
		"maxrss: %ld KB  majflt: %ld  inblk: %ld  outblk: %ld\n",
		rus.ru_maxrss, rus.ru_majflt, rus.ru_inblock, rus.ru_oublock);
	rc += buff;

	// The above chews up 7 lines of display. Byobu/tmux needs a line.
	// Blank line for accepting commands. So subtract 9.
	rc += "\n";
	rc += display_stats(nlines - 9);

	return rc;
}

std::string SocketManager::display_stats(int nlines)
{
	// Hack(?) Send a half-ping, in an attempt to close
	// dead connections.
	half_ping();

	// Current max sockets is 20
	// A standard terminal 24 rows x 80 columns is 1920 bytes.
	std::string rc;
	rc.reserve(2000);

	// Report under a lock so that sockets don't change while
	// we access them.
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
	int nprt = 0;
	for (ServerSocket* ss : sov)
	{
		if (0 == nprt)
		{
			rc += ss->connection_header() + "\n";
			nprt ++;
		}
		rc += ss->connection_stats() + "\n";
		nprt ++;

		// Print at most nlines; negative nlines is unlimited.
		if (0 < nlines and nlines < nprt) break;
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
void SocketManager::half_ping(void)
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
		if (ss->_status == ServerSocket::IWAIT and
			now - ss->_last_activity > 10)
		{
			if (ss->_do_frame_io)
				ss->send_websocket_pong();
			else
				ss->Send(buf);
		}
	}
}

/// Kill the indicated thread id.
// TODO: should use std::jthread, once c++20 is widely available.
bool SocketManager::kill(pid_t tid)
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

// Wait for all shells to finish evaluating pending commands.
// This provides a barrier/fence for synchronization between
// fire-and-forget command streams sent on different connections.
void SocketManager::barrier()
{
	// Find out which of these socekts is ourself.
	ServerSocket* our_socket = nullptr;
	{
		std::lock_guard<std::mutex> lock(_sock_lock);
		for (ServerSocket* ss : _sock_list)
		{
			ConsoleSocket* cs = dynamic_cast<ConsoleSocket*>(ss);
			if (cs)
			{
				GenericShell* shell = cs->getShell();
				if (shell and shell->is_eval_thread())
				{
					our_socket = ss;
					break;
				}
			}
		}
	}
	OC_ASSERT(nullptr != our_socket, "Barrier called out-of-band!");

	our_socket->_in_barrier = true;
	our_socket->_status = ServerSocket::BAR;

	// Set barrier active to block new work from being enqueued
	{
		std::lock_guard<std::mutex> lock(_barrier_mtx);
		_barrier_active = true;
	}

	// Loop until all shells have drained thier work queues.
	while (true)
	{
		bool all_idle = true;
		{
			std::lock_guard<std::mutex> lock(_sock_lock);
			for (ServerSocket* ss : _sock_list)
			{
				if (ss == our_socket)
					continue;

				// There might be multiple concurrent bars. Avoid deadlock.
				if (ss->_in_barrier)
					continue;

				ConsoleSocket* cs = dynamic_cast<ConsoleSocket*>(ss);
				if (cs and cs->busyShell())
				{
					all_idle = false;
					break;
				}
			}
		}

		if (all_idle)
			break;

		// Sleep briefly before checking again
		usleep(10000); // 10ms
	}

	our_socket->_in_barrier = false;
	std::lock_guard<std::mutex> lock(_sock_lock);
	for (ServerSocket* ss : _sock_list)
		if (ss->_in_barrier) return;

	// Release barrier only if we are the last one out.
	{
		std::lock_guard<std::mutex> lock(_barrier_mtx);
		_barrier_active = false;
		_barrier_cv.notify_all();
	}
}

/// Prevent shells from enqueueing new work.
void SocketManager::block_on_bar()
{
	std::unique_lock<std::mutex> lock(_barrier_mtx);
	while (_barrier_active)
	{
		_barrier_cv.wait(lock);
	}
}

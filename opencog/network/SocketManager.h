/*
 * opencog/network/SocketManager.h
 *
 * Copyright (C) 2025 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_SOCKET_MANAGER_H
#define _OPENCOG_SOCKET_MANAGER_H

#include <condition_variable>
#include <mutex>
#include <set>

namespace opencog
{

class ServerSocket;
class ConsoleSocket;
class GenericShell;

/**
 * SocketManager manages all active socket connections.
 * Tracks open sockets, enforces connection limits, and provides
 * synchronization primitives like barrier for coordinating
 * across multiple socket connections.
 */
class SocketManager
{
	friend class ServerSocket;
	friend class GenericShell;   // Calls block_on_bar()

private:
	// Socket registry
	std::mutex _sock_lock;
	std::set<ServerSocket*> _sock_list;

	// Connection limiting
	unsigned int _max_open_sockets;
	volatile unsigned int _num_open_sockets;
	std::mutex _max_mtx;
	std::condition_variable _max_cv;
	size_t _num_open_stalls;

	// Barrier synchronization
	std::mutex _barrier_mtx;
	std::condition_variable _barrier_cv;
	bool _barrier_active;

	// Global flags
	bool _network_gone;

	// Internal helper methods
	void half_ping();
	std::string display_stats(int nlines);

protected:
	// Methods for ServerSocket (friend class) to manage its lifecycle
	void add_sock(ServerSocket*);
	void rem_sock(ServerSocket*);
	void wait_available_slot();
	void release_slot();
	bool is_network_gone() const { return _network_gone; }

	// Bar shells from enqueueing new work.
	void block_on_bar();

public:
	SocketManager();
	~SocketManager();

	// Configuration
	void set_max_open_sockets(unsigned int m) { _max_open_sockets = m; }

	// Network status control
	void network_gone() { _network_gone = true; }

	// Public socket operations
	std::string display_stats_full(const char* title, time_t start_time, int nlines = -1);
	bool kill(pid_t tid);

	/**
	 * Synchronization point. Barrier-fence. Force shells to drain thier
	 * pending-work queues. Shell sockets will not be able to enqueue
	 * new work, until after all work queues have drained.
	 *
	 * This is a "global" sync point, synchronizing all shells. The
	 * current implementation is simple: calling this will block until
	 * all shells have drained. The block_on_bar() below prevents shells
	 * from enqueueing new work until all of them have drained.
	 */
	void barrier();
};

} // namespace

#endif // _OPENCOG_SOCKET_MANAGER_H

/*
 * FILE:
 * opencog/persist/cog-storage/CogStorage.cc
 *
 * FUNCTION:
 * Simple CogServer-backed persistent storage.
 *
 * HISTORY:
 * Copyright (c) 2020 Linas Vepstas <linasvepstas@gmail.com>
 *
 * LICENSE:
 * SPDX-License-Identifier: AGPL-3.0-or-later
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "CogStorage.h"

using namespace opencog;

/// This is a cheap, simple, super-low-brow atomspace server
/// built on the cogserver. Its not special. It's simple.
/// It is meant to be replaced by something better.

/* ================================================================ */
// Constructors

void CogStorage::init(const char * uri)
{
#define URIX_LEN (sizeof("cog://") - 1)  // Should be 6
	if (strncmp(uri, "cog://", URIX_LEN))
		throw IOException(TRACE_INFO, "Unknown URI '%s'\n", uri);

	_uri = uri;

	// We expect the URI to be for the form
	//    cog://ipv4-addr/atomspace-name
	//    cog://ipv4-addr:port/atomspace-name

	std::string host(uri + URIX_LEN);
	size_t slash = host.find_first_of(":/");
	if (std::string::npos != slash)
		host = host.substr(0, slash);

#define DEFAULT_COGSERVER_PORT "17001"
	std::string port = DEFAULT_COGSERVER_PORT;
	size_t colon = _uri.find(':', URIX_LEN + host.length());
	if (std::string::npos != colon)
	{
		port = _uri.substr(colon+1);
		slash = port.find('/');
		if (std::string::npos != slash)
			port = port.substr(0, slash);
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *servinfo;
	int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &servinfo);
	if (rc)
		throw IOException(TRACE_INFO, "Unknown host %s: %s",
			host.c_str(), strerror(rc));

	_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

	if (0 > _sockfd)
	{
		int norr = errno;
		free(servinfo);
		throw IOException(TRACE_INFO, "Unable to create socket to host %s: %s",
			host.c_str(), strerror(norr));
	}

	rc = connect(_sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (0 > rc)
	{
		int norr = errno;
		free(servinfo);
		throw IOException(TRACE_INFO, "Unable to connect to host %s: %s",
			host.c_str(), strerror(norr));
	}

	free(servinfo);

	// Get to the scheme prompt, but make it be silent.
	std::string eval = "scm hush\n";
	rc = send(_sockfd, eval.c_str(), eval.size(), 0);
	if (0 > rc)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver at host %s: %s",
			host.c_str(), strerror(errno));

	// Throw away the cogserver prompt.
	do_recv();
}

CogStorage::CogStorage(std::string uri)
	: _sockfd(-1)
{
	init(uri.c_str());
}

CogStorage::~CogStorage()
{
	if (connected())
		close(_sockfd);
}

bool CogStorage::connected(void)
{
	return 0 < _sockfd;
}

/* ================================================================== */

void CogStorage::do_send(const std::string& str)
{
	if (not connected())
		throw IOException(TRACE_INFO, "Not connected to cogserver!");

	int rc = send(_sockfd, str.c_str(), str.size(), 0);
	if (0 > rc)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
			strerror(errno));
}

std::string CogStorage::do_recv()
{
	if (not connected())
		throw IOException(TRACE_INFO, "Not connected to cogserver!");

	// Receive 4K bytes of message.
	// XXX FIXME Clearly, this fails is the result is larger than
	// 4K. One possible fix is to loop while len == 4096. Another
	// possible fix is have do_send() send something that causes
	// the cogserver to write some end-of-message mark. For now,
	// we are keeping it super-ultra-simple.
	char buf[4097];
	int len = recv(_sockfd, buf, 4096, 0);

	if (0 > len)
		throw IOException(TRACE_INFO, "Unable to talk to cogserver: %s",
			strerror(errno));
	if (0 == len)
	{
		close(_sockfd);
		_sockfd = 0;
		throw IOException(TRACE_INFO, "Cogserver unexpectedly closed connection");
	}

	buf[len] = 0;
	return buf;
}

/* ================================================================== */
/// Drain the pending store queue. This is a fencing operation; the
/// goal is to make sure that all writes that occurred before the
/// barrier really are performed before before all the writes after
/// the barrier.
///
void CogStorage::barrier()
{
}

/* ================================================================ */

void CogStorage::registerWith(AtomSpace* as)
{
	BackingStore::registerWith(as);
}

void CogStorage::unregisterWith(AtomSpace* as)
{
	if (connected())
		close(_sockfd);
	_sockfd = -1;

	BackingStore::unregisterWith(as);
}

/* ================================================================ */

void CogStorage::clear_stats(void)
{
}

void CogStorage::print_stats(void)
{
	printf("no stats yet\n");
}

/* ============================= END OF FILE ================= */

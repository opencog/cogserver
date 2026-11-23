/*
 * opencog/cogserver/server/NineServer.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <map>
#include <mutex>

#include <opencog/util/exceptions.h>
#include <opencog/util/Logger.h>

#include <opencog/cogserver/server/NineServer.h>

using namespace opencog;

// Each AtomSpace gets exactly one AtomFsIndex, shared by every
// *live* connection serving that AtomSpace, so that the same atom
// always maps to the same NineNode (and thus the same Qid)
// regardless of which connection is asking. The map holds weak
// references: when the last connection for an AtomSpace closes, its
// index (and its grip on the AtomSpace) is released rather than
// pinned forever. Qids are deterministic, so a rebuilt index serves
// identical Qids.
static plan9::AtomFsIndexPtr get_index(const AtomSpacePtr& as)
{
	static std::mutex mtx;
	static std::map<AtomSpace*, std::weak_ptr<plan9::AtomFsIndex>> indexes;

	std::lock_guard<std::mutex> lock(mtx);
	for (auto it = indexes.begin(); it != indexes.end(); )
		it = it->second.expired() ? indexes.erase(it) : std::next(it);

	plan9::AtomFsIndexPtr idx = indexes[as.get()].lock();
	if (nullptr == idx)
	{
		idx = std::make_shared<plan9::AtomFsIndex>(as);
		indexes[as.get()] = idx;
	}
	return idx;
}

NineServer::NineServer(const Handle& hcsn, SocketManager* mgr) :
	ConsoleSocket(mgr),
	_hcsn(hcsn)
{
}

NineServer::~NineServer()
{
	logger().info("9P client disconnected");
}

// ==================================================================

// Called before any data is sent/received.
void NineServer::OnConnection(void)
{
	logger().info("9P client connected");

	// Hold the index for the lifetime of the connection: it must
	// outlive the session (and every node the session's fids hold).
	_index = get_index(AtomSpaceCast(_hcsn->getAtomSpace()));
	_session.reset(new plan9::NineSession(_index->root()));
}

// Called for each complete 9P frame received (the socket is in
// act_as_9p() framing mode, so `line` is one raw message, size
// prefix included).
void NineServer::OnLine(const std::string& line)
{
	try
	{
		Send(_session->handle_message(line));
	}
	catch (const std::exception& e)
	{
		// A frame that cannot even be decoded means the peer is not
		// speaking 9P; drop the connection, per common practice.
		logger().warn("9P: dropping connection: %s", e.what());
		throw SilentException();
	}
}

// ==================================================================

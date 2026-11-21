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
// connection serving that AtomSpace, so that the same atom always
// maps to the same NineNode (and thus the same Qid) regardless of
// which connection is asking.
static plan9::AtomFsIndexPtr get_index(const AtomSpacePtr& as)
{
	static std::mutex mtx;
	static std::map<AtomSpace*, plan9::AtomFsIndexPtr> indexes;

	std::lock_guard<std::mutex> lock(mtx);
	plan9::AtomFsIndexPtr& idx = indexes[as.get()];
	if (nullptr == idx)
		idx = std::make_shared<plan9::AtomFsIndex>(as);
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

	plan9::AtomFsIndexPtr index =
		get_index(AtomSpaceCast(_hcsn->getAtomSpace()));
	_session.reset(new plan9::NineSession(index->root()));
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

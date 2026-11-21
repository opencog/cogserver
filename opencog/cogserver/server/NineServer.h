/*
 * opencog/cogserver/server/NineServer.h
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_NINE_SERVER_H
#define _OPENCOG_NINE_SERVER_H

#include <memory>
#include <string>

#include <opencog/network/ConsoleSocket.h>
#include <opencog/cogserver/9p/AtomFs.h>
#include <opencog/cogserver/9p/NineSession.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a Plan 9 (9P2000) fileserver connection,
 * exposing the CogServer's AtomSpace as a synthetic file tree (see
 * opencog/cogserver/9p/AtomFs.h for the tree layout). One instance
 * is created per network connection; the socket runs in 9P framing
 * mode (act_as_9p()), so OnLine() receives one complete raw 9P frame
 * at a time.
 */
class NineServer : public ConsoleSocket
{
private:
	Handle _hcsn;
	// Declared before _session: the session (whose fids hold nodes
	// handed out by the index) must be destroyed first.
	plan9::AtomFsIndexPtr _index;
	std::unique_ptr<plan9::NineSession> _session;

protected:
	virtual void OnConnection(void);
	virtual void OnLine(const std::string&);

public:
	NineServer(const Handle&, SocketManager*);
	~NineServer();

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_NINE_SERVER_H

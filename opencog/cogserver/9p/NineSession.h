/*
 * opencog/cogserver/9p/NineSession.h
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_NINE_SESSION_H
#define _OPENCOG_NINE_SESSION_H

#include <string>
#include <unordered_map>
#include <vector>

#include <opencog/cogserver/9p/Nine9P.h>
#include <opencog/cogserver/9p/NineNamespace.h>
#include <opencog/cogserver/9p/NineNode.h>

namespace opencog
{
namespace plan9
{
/** \addtogroup grp_server
 *  @{
 */

struct NineFid
{
	std::string path;
	NineNodePtr node;
	// The union stack resolved when this fid was attached/walked.
	// Directory reads use this captured view, so later namespace
	// rearrangements (mount/unmount) affect new walks only, never
	// the target of an already-established fid.
	std::vector<NineNodePtr> stack;
	uint8_t mode = 0;
	bool opened = false;
	/// Snapshot of this directory's encoded listing, (re)taken at
	/// each Tread at offset 0, through which subsequent sequential
	/// reads page (read(5) directory semantics).
	std::string dir_snapshot;
};

/**
 * One 9P2000 protocol session: the per-connection fid table and
 * message dispatcher, serving the file tree visible through a
 * NineNamespace. Each network connection gets its own NineSession
 * (and with it, its own private copy of the namespace, so binds and
 * mounts in one session never affect another); the underlying node
 * trees (see AtomFs.h) are shared between them.
 *
 * The session owns no file semantics of its own. It enforces the
 * protocol rules (fid lifecycle, open modes, msize, directory read
 * paging) and hands every change to the node concerned: Twrite to
 * NineNode::write(), Topen(OTRUNC) and a length-changing Twstat to
 * truncate(), Tcreate to create(), Tremove to remove(). A node
 * that does not override those is immutable, and the request is
 * refused. Twstat can change nothing else: renames and changes of
 * mode or ownership are refused, while a Twstat that changes
 * nothing (including the all-"don't touch" sync form) succeeds.
 *
 * Every Rerror carries one of the NINE_ERR_* strings of Nine9P.h,
 * which the Linux v9fs client can map to an errno; nodes signal
 * failure by throwing a NineError.
 */
class NineSession
{
public:
	/// Serve the tree rooted at `root`, with no additional binds.
	explicit NineSession(NineNodePtr root);

	/// Serve a private copy of `ns` (Plan 9 rfork(RFNAMEG) style).
	explicit NineSession(const NineNamespace& ns);

	/// This session's namespace, for programmatic bind()/mount()
	/// composition before (or while) serving.
	NineNamespace& ns() { return _ns; }

	/// Process one already-decoded 9P message and produce the reply.
	NineMsg dispatch(const NineMsg& req);

	/// Decode exactly one complete message from `request`, dispatch
	/// it, and return the encoded reply. Throws std::runtime_error
	/// if `request` does not hold a complete message (callers
	/// streaming from a socket should frame messages before calling
	/// this).
	std::string handle_message(const std::string& request);

private:
	NineNamespace _ns;
	std::unordered_map<uint32_t, NineFid> _fids;
	uint32_t _msize = NINE_DEFAULT_MSIZE;

	NineMsg do_version(const NineMsg&);
	NineMsg do_attach(const NineMsg&);
	NineMsg do_walk(const NineMsg&);
	NineMsg do_open(const NineMsg&);
	NineMsg do_create(const NineMsg&);
	NineMsg do_read(const NineMsg&);
	NineMsg do_write(const NineMsg&);
	NineMsg do_clunk(const NineMsg&);
	NineMsg do_remove(const NineMsg&);
	NineMsg do_stat(const NineMsg&);
	NineMsg do_wstat(const NineMsg&);
	NineMsg do_flush(const NineMsg&);

	static std::string parent_path(const std::string& path);
	static std::string join_path(const std::string& path, const std::string& c);
	static NineQid qid_of(const NineNodePtr&);
	NineStat stat_for(const std::string& path, const NineNodePtr&) const;
};

/** @}*/
} // namespace plan9
} // namespace opencog

#endif // _OPENCOG_NINE_SESSION_H

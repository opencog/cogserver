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
	uint8_t mode = 0;
	bool opened = false;
	/// Snapshot of this directory's encoded listing, (re)taken at
	/// each Tread at offset 0, through which subsequent sequential
	/// reads page (read(5) directory semantics).
	std::string dir_snapshot;
};

/**
 * One 9P2000 protocol session: the per-connection fid table and
 * message dispatcher, serving the synthetic file tree rooted at a
 * single NineNode. Each network connection gets its own NineSession;
 * the underlying node tree (see AtomFs.h) is shared between them.
 *
 * The tree is structurally read-only: Tcreate and Twstat are
 * rejected, and Tremove is refused with Rerror (though the fid is
 * still clunked, per remove(5)).
 * Individual files may still accept Twrite (e.g. the per-atom `tv`
 * file) when their node reports is_writable().
 */
class NineSession
{
public:
	explicit NineSession(NineNodePtr root);

	/// Process one already-decoded 9P message and produce the reply.
	NineMsg dispatch(const NineMsg& req);

	/// Decode exactly one complete message from `request`, dispatch
	/// it, and return the encoded reply. Throws std::runtime_error
	/// if `request` does not hold a complete message (callers
	/// streaming from a socket should frame messages before calling
	/// this).
	std::string handle_message(const std::string& request);

private:
	NineNodePtr _root;
	std::unordered_map<uint32_t, NineFid> _fids;
	uint32_t _msize = NINE_DEFAULT_MSIZE;

	NineMsg do_version(const NineMsg&);
	NineMsg do_attach(const NineMsg&);
	NineMsg do_walk(const NineMsg&);
	NineMsg do_open(const NineMsg&);
	NineMsg do_read(const NineMsg&);
	NineMsg do_write(const NineMsg&);
	NineMsg do_clunk(const NineMsg&);
	NineMsg do_remove(const NineMsg&);
	NineMsg do_stat(const NineMsg&);
	NineMsg do_flush(const NineMsg&);

	/// Resolve a canonical absolute path to its node, starting from
	/// _root; returns nullptr if any component is missing.
	NineNodePtr resolve(const std::string& path) const;

	static std::vector<std::string> split(const std::string& path);
	static std::string canon(const std::string& path);
	static std::string parent_path(const std::string& path);
	static std::string join_path(const std::string& path, const std::string& c);
	static NineQid qid_of(const NineNodePtr&);
	NineStat stat_for(const std::string& path, const NineNodePtr&) const;
};

/** @}*/
} // namespace plan9
} // namespace opencog

#endif // _OPENCOG_NINE_SESSION_H

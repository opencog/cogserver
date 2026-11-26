/*
 * opencog/cogserver/9p/NineSession.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/cogserver/9p/NineSession.h>

#include <algorithm>
#include <stdexcept>

#include <opencog/util/Logger.h>

namespace opencog
{
namespace plan9
{

NineSession::NineSession(NineNodePtr root) : _ns(std::move(root))
{}

NineSession::NineSession(const NineNamespace& ns) : _ns(ns)
{}

// ---------------------------------------------------------------
// Path helpers.

std::string NineSession::parent_path(const std::string& path)
{
	std::string canonical = NineNamespace::canon(path);
	size_t slash = canonical.find_last_of('/');
	if (slash == std::string::npos or slash == 0)
		return "/";
	return canonical.substr(0, slash);
}

std::string NineSession::join_path(const std::string& path, const std::string& c)
{
	if (path == "/") return "/" + c;
	return path + "/" + c;
}

NineQid NineSession::qid_of(const NineNodePtr& node)
{
	if (nullptr == node)
	{
		NineQid q; q.type = QTFILE; q.path = 0; return q;
	}
	return node->qid();
}

NineStat NineSession::stat_for(const std::string& path,
                               const NineNodePtr& node) const
{
	NineStat st;
	st.qid = qid_of(node);
	bool is_dir = node and node->is_dir();
	st.mode = is_dir ? (NINE_DMDIR | 0555u) : 0444u;
	if (node and node->is_writable())
		st.mode |= 0222u;
	st.length = (is_dir or nullptr == node) ? 0 : node->length();
	st.uid = "cogserver";
	st.gid = "cogserver";
	st.muid = "cogserver";

	if (path == "/")
	{
		st.name = "/";
	}
	else
	{
		size_t slash = path.find_last_of('/');
		st.name = (slash == std::string::npos) ? path : path.substr(slash + 1);
	}
	return st;
}

// ---------------------------------------------------------------
// Dispatch.

NineMsg NineSession::dispatch(const NineMsg& req)
{
	try
	{
		switch (req.type)
		{
			case Tversion: return do_version(req);
			// No Linux errno fits "no authentication needed", and a
			// client that needs none never sends Tauth; the text is
			// for people (attach(5)).
			case Tauth:    return nine_error(req.tag,
				"authentication not required");
			case Tattach:  return do_attach(req);
			case Tflush:   return do_flush(req);
			case Twalk:    return do_walk(req);
			case Topen:    return do_open(req);
			case Tcreate:  return do_create(req);
			case Tread:    return do_read(req);
			case Twrite:   return do_write(req);
			case Tclunk:   return do_clunk(req);
			case Tremove:  return do_remove(req);
			case Tstat:    return do_stat(req);
			case Twstat:   return do_wstat(req);
			default:
				return nine_error(req.tag, NINE_ERR_NOT_IMPL);
		}
	}
	catch (const NineError& e)
	{
		// The wire carries only the canonical text; what actually
		// went wrong is kept for whoever reads the log.
		if (not e.detail().empty())
			logger().debug("9P: %s: %s", e.what(), e.detail().c_str());
		return nine_error(req.tag, e.what());
	}
	catch (const std::exception& e)
	{
		// Anything else is an internal failure whose text no client
		// could interpret.
		logger().info("9P: message type %d failed: %s",
			(int) req.type, e.what());
		return nine_error(req.tag, NINE_ERR_IO);
	}
}

std::string NineSession::handle_message(const std::string& request)
{
	NineMsg req;
	size_t used = nine_decode(request, req);
	if (0 == used)
		throw std::runtime_error("9P: incomplete message");
	// The negotiated msize bounds frames in both directions
	// (version(5)); reject oversized requests rather than
	// processing them.
	NineMsg rep = (used > _msize)
		? nine_error(req.tag, NINE_ERR_TOO_LARGE)
		: dispatch(req);
	return nine_encode(rep);
}

NineMsg NineSession::do_version(const NineMsg& req)
{
	// Refuse an msize too small to carry useful replies, rather
	// than negotiating an unusable session.
	if (req.msize < NINE_MIN_MSIZE)
		return nine_error(req.tag, NINE_ERR_INVALID);

	// Version negotiation (re)starts the session: any fids from an
	// earlier negotiation are clunked, per version(5).
	_fids.clear();

	NineMsg rep;
	rep.type = Rversion;
	rep.tag = req.tag;
	_msize = std::min(req.msize, NINE_DEFAULT_MSIZE);
	rep.msize = _msize;
	rep.version = (req.version.substr(0, 6) == "9P2000")
		? std::string(NINE_VERSION) : std::string("unknown");
	return rep;
}

NineMsg NineSession::do_attach(const NineMsg& req)
{
	// A fid already in use may not be rebound (attach(5)).
	if (_fids.count(req.fid))
		return nine_error(req.tag, NINE_ERR_FID_IN_USE);

	std::string path = req.aname.empty() ? "/" : NineNamespace::canon(req.aname);
	std::vector<NineNodePtr> stack = _ns.walk_union(path);
	if (stack.empty() or nullptr == stack.front())
		return nine_error(req.tag, NINE_ERR_NOT_FOUND);
	NineNodePtr node = stack.front();

	_fids[req.fid] = NineFid{ path, node, std::move(stack), 0, false };

	NineMsg rep;
	rep.type = Rattach;
	rep.tag = req.tag;
	rep.qid = qid_of(node);
	return rep;
}

NineMsg NineSession::do_walk(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);
	// The source fid must not have been opened for I/O, and the
	// destination must be free unless it is fid itself (walk(5)).
	if (it->second.opened)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);
	if (req.newfid != req.fid and _fids.count(req.newfid))
		return nine_error(req.tag, NINE_ERR_FID_IN_USE);

	std::string path = it->second.path;
	NineNodePtr node = it->second.node;
	// A pure clone (empty wname) keeps the source fid's captured
	// union view; walking components re-resolves through the
	// current namespace.
	std::vector<NineNodePtr> stack = it->second.stack;
	std::vector<NineQid> qids;

	for (const auto& comp : req.wname)
	{
		// Every step, including "..", walks *from* a directory.
		if (nullptr == node or not node->is_dir())
			break;
		// Each component re-resolves through the mount table, so a
		// tree grafted anywhere along the path is honored; ".."
		// resolves the (strictly shorter) parent path, which nodes
		// do not record.
		std::string candidate =
			(comp == "..") ? parent_path(path) : join_path(path, comp);
		std::vector<NineNodePtr> next = _ns.walk_union(candidate);
		if (next.empty() or nullptr == next.front())
			break; // Partial walk; per 9P this is not itself an error.
		path = candidate;
		stack = std::move(next);
		node = stack.front();
		qids.push_back(qid_of(node));
	}

	if (not req.wname.empty() and qids.empty())
		return nine_error(req.tag, NINE_ERR_NOT_FOUND);

	NineMsg rep;
	rep.type = Rwalk;
	rep.tag = req.tag;
	rep.wqid = qids;

	// A full (or trivially empty, i.e. clone) walk commits to newfid;
	// a partial walk reports what it found without creating newfid,
	// matching walk(5)'s semantics.
	if (qids.size() == req.wname.size())
		_fids[req.newfid] = NineFid{ path, node, std::move(stack), 0, false };

	return rep;
}

// The open-mode rules shared by Topen and Tcreate: flags whose
// required behavior this server cannot provide are refused rather
// than silently ignored (open(5)). Returns the error text, or
// nullptr if `mode` is acceptable.
static const char* check_mode(uint8_t mode)
{
	// Remove-on-clunk would make a mere close destructive.
	if (mode & ORCLOSE) return NINE_ERR_PERM;
	uint8_t rw = mode & 3;
	if (OEXEC == rw) return NINE_ERR_BAD_MODE;
	// Truncating is a modification: it needs a write mode.
	if ((mode & OTRUNC) and OWRITE != rw and ORDWR != rw)
		return NINE_ERR_BAD_MODE;
	return nullptr;
}

NineMsg NineSession::do_open(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);
	// A fid already opened by a successful open may not be opened
	// again (open(5)).
	if (it->second.opened)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);

	if (const char* bad = check_mode(req.mode))
		return nine_error(req.tag, bad);

	NineNodePtr node = it->second.node;
	uint8_t rw = req.mode & 3;
	bool wants_write = (rw == OWRITE or rw == ORDWR);
	if (wants_write and node and node->is_dir())
		return nine_error(req.tag, NINE_ERR_IS_DIR);
	if (wants_write and not (node and node->is_writable()))
		return nine_error(req.tag, NINE_ERR_PERM);

	// The node decides what truncation means for it (and may throw
	// to refuse, leaving the fid unopened).
	if (req.mode & OTRUNC)
		node->truncate(0);

	it->second.opened = true;
	it->second.mode = req.mode;

	NineMsg rep;
	rep.type = Ropen;
	rep.tag = req.tag;
	rep.qid = qid_of(node);
	rep.iounit = 0;
	return rep;
}

NineMsg NineSession::do_create(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);
	NineFid& f = it->second;
	// The fid must be an unopened directory (create(5)).
	if (f.opened)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);
	if (nullptr == f.node or not f.node->is_dir())
		return nine_error(req.tag, NINE_ERR_NOT_DIR);

	// "." and ".." are illegal names (create(5)), and no path
	// element may contain a slash.
	if (req.name.empty() or req.name == "." or req.name == ".." or
	    req.name.find('/') != std::string::npos)
		return nine_error(req.tag, NINE_ERR_INVALID);
	if (const char* bad = check_mode(req.mode))
		return nine_error(req.tag, bad);

	// Creating a name that is already visible here is an error.
	std::string path = join_path(f.path, req.name);
	if (_ns.walk(path))
		return nine_error(req.tag, NINE_ERR_EXISTS);

	// In a union directory the file is made in the first member,
	// which is the one this fid's node refers to. The node decides
	// whether, and what, it can create; it throws to refuse.
	NineNodePtr made = f.node->create(req.name, req.perm);
	if (nullptr == made)
		return nine_error(req.tag, NINE_ERR_IO);

	// The fid now represents the new file, opened with `mode`.
	f.path = path;
	f.node = made;
	f.stack = { made };
	f.mode = req.mode;
	f.opened = true;
	f.dir_snapshot.clear();

	NineMsg rep;
	rep.type = Rcreate;
	rep.tag = req.tag;
	rep.qid = qid_of(made);
	rep.iounit = 0;
	return rep;
}

NineMsg NineSession::do_read(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);
	if (not it->second.opened)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);
	if (OWRITE == (it->second.mode & 3))
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);

	// An Rread header is size[4] type[1] tag[2] count[4] = 11 bytes;
	// the payload must keep the whole frame within the negotiated
	// msize, whatever count the client asked for.
	uint32_t count = std::min(req.count,
		(_msize > 11) ? _msize - 11 : 0);

	NineMsg rep;
	rep.type = Rread;
	rep.tag = req.tag;

	NineNodePtr node = it->second.node;
	if (node and node->is_dir())
	{
		NineFid& f = it->second;

		// A read at offset 0 (re)takes a snapshot of the whole
		// listing; sequential reads then page through the snapshot,
		// each returning an integral number of stat entries, per
		// the read(5) directory contract.
		if (0 == req.offset)
		{
			// The listing is the union view captured when this fid
			// was walked, so mounts/unmounts made since then do not
			// retarget an open directory; per-child mount overrides
			// still resolve through the current namespace so dirent
			// metadata matches Rwalk.
			std::vector<NineDirEnt> kids =
				_ns.list_union(f.path, f.stack);
			std::string blob;
			for (const auto& k : kids)
			{
				if (not k.node) continue;
				std::string childpath = join_path(f.path, k.name);
				blob += nine_encode_stat(stat_for(childpath, k.node));
			}
			f.dir_snapshot = std::move(blob);
		}

		const std::string& snap = f.dir_snapshot;
		if (req.offset > snap.size())
			return nine_error(req.tag, NINE_ERR_DIR_OFFSET);

		// Take whole stat entries (each self-describing via its
		// leading size[2], which does not count itself) up to count.
		size_t avail = snap.size() - (size_t) req.offset;
		size_t limit = std::min(avail, (size_t) count);
		size_t take = 0;
		while (take + 2 <= limit)
		{
			size_t at = (size_t) req.offset + take;
			size_t entry = 2 + ((size_t) (uint8_t) snap[at]
				| ((size_t) (uint8_t) snap[at+1] << 8));
			if (take + entry > limit) break;
			take += entry;
		}
		// A count too small to hold even one entry cannot make
		// progress; an empty Rread would wrongly signal EOF.
		if (0 == take and 0 < avail)
			return nine_error(req.tag, NINE_ERR_INVALID);
		rep.data = snap.substr(req.offset, take);
		return rep;
	}

	rep.data = node ? node->read(req.offset, count) : std::string();
	return rep;
}

NineMsg NineSession::do_write(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);
	if (not it->second.opened)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);
	uint8_t rw = it->second.mode & 3;
	if (OWRITE != rw and ORDWR != rw)
		return nine_error(req.tag, NINE_ERR_BAD_FID_USE);
	NineNodePtr node = it->second.node;
	if (nullptr == node or node->is_dir())
		return nine_error(req.tag, NINE_ERR_IS_DIR);
	// A Twrite header is size[4] type[1] tag[2] fid[4] offset[8]
	// count[4] = 23 bytes; a compliant client never exceeds msize.
	if (23 + req.data.size() > _msize)
		return nine_error(req.tag, NINE_ERR_TOO_LARGE);

	// The node throws to reject the write, which is then reported
	// as an Rerror, as write(5) requires, rather than as a
	// misleading zero-count Rwrite.
	uint32_t n = node->write(req.offset, req.data);

	NineMsg rep;
	rep.type = Rwrite;
	rep.tag = req.tag;
	rep.count = n;
	return rep;
}

NineMsg NineSession::do_clunk(const NineMsg& req)
{
	_fids.erase(req.fid);
	NineMsg rep;
	rep.type = Rclunk;
	rep.tag = req.tag;
	return rep;
}

NineMsg NineSession::do_remove(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);

	// A Tremove clunks its fid even when the removal is refused
	// (remove(5)), so the fid goes before the node gets its say.
	NineNodePtr node = it->second.node;
	_fids.erase(it);
	if (nullptr == node)
		return nine_error(req.tag, NINE_ERR_NOT_FOUND);

	// The node throws to refuse; most of this tree does.
	node->remove();

	NineMsg rep;
	rep.type = Rremove;
	rep.tag = req.tag;
	return rep;
}

NineMsg NineSession::do_stat(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);

	NineMsg rep;
	rep.type = Rstat;
	rep.tag = req.tag;
	rep.stat = nine_encode_stat(stat_for(it->second.path, it->second.node));
	return rep;
}

NineMsg NineSession::do_wstat(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, NINE_ERR_UNKNOWN_FID);

	NineStat want;
	try
	{
		want = nine_decode_stat(req.stat);
	}
	catch (const std::exception&)
	{
		return nine_error(req.tag, NINE_ERR_BAD_WSTAT);
	}

	NineNodePtr node = it->second.node;
	const NineStat cur = stat_for(it->second.path, node);

	// A field is left alone when it holds its "don't touch" value
	// -- all ones for an integer, the empty string for text
	// (stat(5)) -- and a field restating the current value is no
	// change either. Whatever is left is a requested change.
	const uint32_t keep32 = ~(uint32_t) 0;
	const uint64_t keep64 = ~(uint64_t) 0;
	auto text = [](const std::string& w, const std::string& c)
	{
		return not w.empty() and w != c;
	};

	// Nothing in this tree can be renamed, re-moded or re-owned,
	// and type, dev and qid are never the client's to set.
	if ((want.type != (uint16_t) keep32 and want.type != cur.type) or
	    (want.dev != keep32 and want.dev != cur.dev) or
	    (want.qid.type != (uint8_t) keep32 and
	        want.qid.type != cur.qid.type) or
	    (want.qid.version != keep32 and
	        want.qid.version != cur.qid.version) or
	    (want.qid.path != keep64 and want.qid.path != cur.qid.path) or
	    (want.mode != keep32 and want.mode != cur.mode) or
	    text(want.name, cur.name) or text(want.uid, cur.uid) or
	    text(want.gid, cur.gid) or text(want.muid, cur.muid))
		return nine_error(req.tag, NINE_ERR_WSTAT);

	bool set_length = (want.length != keep64 and want.length != cur.length);
	bool set_times =
		(want.mtime != keep32 and want.mtime != cur.mtime) or
		(want.atime != keep32 and want.atime != cur.atime);

	// A Twstat that changes nothing always succeeds. With every
	// field "don't touch" it is a request to commit the file to
	// stable storage (which is how Linux implements fsync); there
	// is nothing to flush, as every write takes effect at once.
	if (set_length or set_times)
	{
		// Changing a file takes write permission on it, and a
		// directory's length cannot be set at all (stat(5)).
		if (nullptr == node or node->is_dir() or not node->is_writable())
			return nine_error(req.tag, NINE_ERR_WSTAT);

		// This is how Linux truncates: opening with O_TRUNC, as a
		// shell `>` redirection does, sends the new length (and
		// usually the modification time) in a Twstat, ahead of
		// the first Twrite. The node decides what truncation
		// means, and throws to refuse.
		if (set_length)
			node->truncate(want.length);

		// No timestamps are kept, so setting one (as touch(1) and
		// the truncation above do) is accepted and has no effect.
	}

	NineMsg rep;
	rep.type = Rwstat;
	rep.tag = req.tag;
	return rep;
}

NineMsg NineSession::do_flush(const NineMsg& req)
{
	// This server is strictly synchronous (dispatch() never blocks
	// past the single request being handled), so there is never an
	// in-flight request to actually cancel; acknowledge immediately.
	NineMsg rep;
	rep.type = Rflush;
	rep.tag = req.tag;
	return rep;
}

} // namespace plan9
} // namespace opencog

/*
 * opencog/cogserver/9p/NineSession.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/cogserver/9p/NineSession.h>

#include <algorithm>
#include <stdexcept>

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
			case Tauth:    return nine_error(req.tag,
				"authentication not required");
			case Tattach:  return do_attach(req);
			case Tflush:   return do_flush(req);
			case Twalk:    return do_walk(req);
			case Topen:    return do_open(req);
			case Tcreate:  return nine_error(req.tag,
				"create not supported (read-only tree)");
			case Tread:    return do_read(req);
			case Twrite:   return do_write(req);
			case Tclunk:   return do_clunk(req);
			case Tremove:  return do_remove(req);
			case Tstat:    return do_stat(req);
			case Twstat:   return nine_error(req.tag, "read-only tree");
			default:
				return nine_error(req.tag, "message type not implemented");
		}
	}
	catch (const std::exception& e)
	{
		return nine_error(req.tag, e.what());
	}
}

std::string NineSession::handle_message(const std::string& request)
{
	NineMsg req;
	size_t used = nine_decode(request, req);
	if (0 == used)
		throw std::runtime_error("9P: incomplete message");
	NineMsg rep = dispatch(req);
	return nine_encode(rep);
}

NineMsg NineSession::do_version(const NineMsg& req)
{
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
	std::string path = req.aname.empty() ? "/" : NineNamespace::canon(req.aname);
	std::vector<NineNodePtr> stack = _ns.walk_union(path);
	if (stack.empty() or nullptr == stack.front())
		return nine_error(req.tag, "attach: no such path: " + req.aname);
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
		return nine_error(req.tag, "walk: unknown fid");

	std::string path = it->second.path;
	NineNodePtr node = it->second.node;
	// A pure clone (empty wname) keeps the source fid's captured
	// union view; walking components re-resolves through the
	// current namespace.
	std::vector<NineNodePtr> stack = it->second.stack;
	std::vector<NineQid> qids;

	for (const auto& comp : req.wname)
	{
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
		return nine_error(req.tag, "walk: file not found");

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

NineMsg NineSession::do_open(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, "open: unknown fid");

	uint8_t rw = req.mode & 3;
	if ((rw == OWRITE or rw == ORDWR) and
	    not (it->second.node and it->second.node->is_writable()))
	{
		return nine_error(req.tag, "open: permission denied");
	}

	it->second.opened = true;
	it->second.mode = req.mode;

	NineMsg rep;
	rep.type = Ropen;
	rep.tag = req.tag;
	rep.qid = qid_of(it->second.node);
	rep.iounit = 0;
	return rep;
}

NineMsg NineSession::do_read(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, "read: unknown fid");
	if (not it->second.opened)
		return nine_error(req.tag, "read: fid not open");

	NineMsg rep;
	rep.type = Rread;
	rep.tag = req.tag;

	NineNodePtr node = it->second.node;
	if (node and node->is_dir())
	{
		// Simplified directory read: the whole listing is produced
		// on the first read (offset 0); subsequent reads at nonzero
		// offset return EOF. This avoids needing to track a stable
		// per-fid cursor across separate Tread calls, at the cost of
		// not supporting incremental paging of very large
		// directories -- acceptable for a synthetic, introspection
		// filesystem, and documented in README.md.
		if (req.offset != 0)
		{
			rep.data.clear();
			return rep;
		}

		// List the union view captured when this fid was walked, so
		// mounts/unmounts made since then do not retarget an open
		// directory; per-child mount overrides still resolve through
		// the current namespace so dirent metadata matches Rwalk.
		std::vector<NineDirEnt> kids =
			_ns.list_union(it->second.path, it->second.stack);
		std::string blob;
		for (const auto& k : kids)
		{
			if (not k.node) continue;
			std::string childpath = join_path(it->second.path, k.name);
			std::string entry = nine_encode_stat(stat_for(childpath, k.node));
			if (blob.size() + entry.size() > req.count)
				break;
			blob += entry;
		}
		rep.data = blob;
		return rep;
	}

	rep.data = node ? node->read(req.offset, req.count) : std::string();
	return rep;
}

NineMsg NineSession::do_write(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, "write: unknown fid");
	if (not it->second.opened)
		return nine_error(req.tag, "write: fid not open");
	if (it->second.node and it->second.node->is_dir())
		return nine_error(req.tag, "write: is a directory");

	NineNodePtr node = it->second.node;
	uint32_t n = node ? node->write(req.offset, req.data) : 0;

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
	// This tree is structurally read-only (atoms/dirs cannot be
	// deleted through it); Tremove still clunks the fid, per remove(5)
	// ("whether or not the remove succeeds, the fid is clunked").
	_fids.erase(req.fid);
	NineMsg rep;
	rep.type = Rremove;
	rep.tag = req.tag;
	return rep;
}

NineMsg NineSession::do_stat(const NineMsg& req)
{
	auto it = _fids.find(req.fid);
	if (it == _fids.end())
		return nine_error(req.tag, "stat: unknown fid");

	NineMsg rep;
	rep.type = Rstat;
	rep.tag = req.tag;
	rep.stat = nine_encode_stat(stat_for(it->second.path, it->second.node));
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

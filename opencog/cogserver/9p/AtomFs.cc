/*
 * opencog/cogserver/9p/AtomFs.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/cogserver/9p/AtomFs.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <opencog/atoms/atom_types/atom_types.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Atom.h>
#include <opencog/atoms/value/FloatValue.h>
#include <opencog/persist/sexpr/Sexpr.h>

namespace opencog
{
namespace plan9
{

// ---------------------------------------------------------------
// Small helpers.

static uint64_t mix_id(uint64_t base, const std::string& salt)
{
	// A boost::hash_combine-style mixer: gives distinct, deterministic
	// Qid.path values for the several synthetic entries hung off of a
	// single atom (name, type, outgoing/, ...) without colliding with
	// the atom's own directory Qid (which uses `base` unmixed).
	uint64_t h2 = (uint64_t) std::hash<std::string>{}(salt);
	return base ^ (h2 + 0x9e3779b97f4a7c15ULL + (base << 6) + (base >> 2));
}

std::string AtomFsIndex::id_of(const Handle& h)
{
	char buf[20];
	snprintf(buf, sizeof(buf), "%016llx",
		(unsigned long long) h.value());
	return std::string(buf);
}

// ---------------------------------------------------------------
// Simple read-only text file backed by a content-generating functor,
// re-evaluated on every read so it always reflects live AtomSpace
// state.

namespace {

class TextFileNode : public NineNode
{
public:
	TextFileNode(std::string name, uint64_t qid_path,
	             std::function<std::string()> content) :
		_name(std::move(name)), _qid_path(qid_path),
		_content(std::move(content))
	{}

	std::string name() const override { return _name; }
	bool is_dir() const override { return false; }
	NineQid qid() const override
	{
		NineQid q; q.type = QTFILE; q.path = _qid_path; return q;
	}
	uint64_t length() const override { return _content().size(); }
	std::string read(uint64_t offset, uint32_t count) const override
	{
		std::string s = _content();
		if (offset >= s.size()) return std::string();
		return s.substr(offset, count);
	}

private:
	std::string _name;
	uint64_t _qid_path;
	std::function<std::string()> _content;
};

/// A generic directory node whose children are supplied by a functor,
/// evaluated fresh on every children()/lookup() call. An optional
/// lookup functor lets directories with many children (by-handle/,
/// by-type/<T>/) resolve a single name directly instead of listing
/// every child -- walks are the hot path of every 9P file access.
class FuncDirNode : public NineNode
{
public:
	FuncDirNode(std::string name, uint64_t qid_path,
	            std::function<std::vector<NineNodePtr>()> lister,
	            std::function<NineNodePtr(const std::string&)> finder =
	                nullptr) :
		_name(std::move(name)), _qid_path(qid_path),
		_lister(std::move(lister)), _finder(std::move(finder))
	{}

	std::string name() const override { return _name; }
	bool is_dir() const override { return true; }
	NineQid qid() const override
	{
		NineQid q; q.type = QTDIR; q.path = _qid_path; return q;
	}
	std::vector<NineNodePtr> children() const override { return _lister(); }
	NineNodePtr lookup(const std::string& child_name) const override
	{
		if (_finder) return _finder(child_name);
		return NineNode::lookup(child_name);
	}

private:
	std::string _name;
	uint64_t _qid_path;
	std::function<std::vector<NineNodePtr>()> _lister;
	std::function<NineNodePtr(const std::string&)> _finder;
};

/// The truth-value convenience file: reads back "<strength>
/// <confidence>" from the atom's truth_key() value (when it is a
/// FloatValue with at least two components), and, when the
/// AtomSpace is writable, accepts writes of the same format to
/// replace it. Updates go through AtomSpace::set_value(), so
/// writing an atom inherited from a read-only parent frame copies
/// it into the exported space (copy-on-write) instead of mutating
/// the parent; the cached handle tracks the returned (local) atom.
class TvFileNode : public NineNode
{
public:
	TvFileNode(AtomSpacePtr as, Handle h, uint64_t qid_path) :
		_as(std::move(as)), _h(std::move(h)), _qid_path(qid_path)
	{}

	std::string name() const override { return "tv"; }
	bool is_dir() const override { return false; }
	NineQid qid() const override
	{
		NineQid q; q.type = QTFILE; q.path = _qid_path; return q;
	}
	uint64_t length() const override { return render().size(); }
	bool is_writable() const override
	{
		return not _as->get_read_only();
	}
	std::string read(uint64_t offset, uint32_t count) const override
	{
		std::string s = render();
		if (offset >= s.size()) return std::string();
		return s.substr(offset, count);
	}
	// Failed writes throw: the dispatcher reports the exception as an
	// Rerror, as write(5) requires, rather than a misleading
	// zero-count Rwrite.
	uint32_t write(uint64_t offset, const std::string& data) override
	{
		if (not is_writable())
			throw std::runtime_error("tv: read-only AtomSpace");
		if (offset != 0)
			throw std::runtime_error("tv: writes must be at offset 0");
		double s = 1.0, c = 0.0;
		if (2 != sscanf(data.c_str(), "%lf %lf", &s, &c))
			throw std::runtime_error(
				"tv: expected \"<strength> <confidence>\"");
		// set_value() performs the COW copy when `_h` lives in a
		// read-only parent frame, and returns the local atom.
		Handle h;
		{
			std::lock_guard<std::mutex> lock(_mtx);
			h = _h;
		}
		Handle local = _as->set_value(h, truth_key(),
			createFloatValue(std::vector<double>({s, c})));
		std::lock_guard<std::mutex> lock(_mtx);
		_h = local;
		return (uint32_t) data.size();
	}

private:
	std::string render() const
	{
		// The node is shared between connections: take a consistent
		// copy of the handle, which write() may be replacing.
		Handle h;
		{
			std::lock_guard<std::mutex> lock(_mtx);
			h = _h;
		}
		FloatValuePtr fv = FloatValueCast(h->getValue(truth_key()));
		if (nullptr == fv or fv->value().size() < 2)
			return "1 0\n";
		std::ostringstream oss;
		oss << fv->value()[0] << " " << fv->value()[1] << "\n";
		return oss.str();
	}

	AtomSpacePtr _as;
	Handle _h;
	uint64_t _qid_path;
	mutable std::mutex _mtx;
};

/// A named alias for another node: same Qid, children, and
/// read/write behaviour as `_target`, but a different directory-entry
/// name. This is how outgoing/incoming "hard links" are implemented:
/// the exact same underlying AtomDir node is reachable under several
/// different names (a positional index under outgoing/, a content-id
/// under incoming/ or by-handle/), and Qid equality (not object
/// identity of the wrapper) is what makes them the "same file".
class AliasNode : public NineNode
{
public:
	AliasNode(std::string name, NineNodePtr target) :
		_name(std::move(name)), _target(std::move(target))
	{}

	std::string name() const override { return _name; }
	bool is_dir() const override { return _target->is_dir(); }
	NineQid qid() const override { return _target->qid(); }
	uint64_t length() const override { return _target->length(); }
	std::vector<NineNodePtr> children() const override
	{
		return _target->children();
	}
	NineNodePtr lookup(const std::string& c) const override
	{
		return _target->lookup(c);
	}
	std::string read(uint64_t offset, uint32_t count) const override
	{
		return _target->read(offset, count);
	}
	uint32_t write(uint64_t offset, const std::string& data) override
	{
		return _target->write(offset, data);
	}
	bool is_writable() const override { return _target->is_writable(); }

private:
	std::string _name;
	NineNodePtr _target;
};

/// Return true iff `data` holds exactly one nonempty, balanced
/// s-expression, optionally surrounded by whitespace. Parentheses
/// inside double-quoted strings (with backslash escapes) do not
/// count. Upstream Sexpr::decode_atom() is laxer than this: it can
/// crash on empty input, accepts unbalanced expressions, and
/// silently ignores trailing text, so we gate writes here instead.
static bool is_single_sexpr(const std::string& data)
{
	size_t i = 0;
	const size_t len = data.size();
	while (i < len and std::isspace((unsigned char) data[i])) i++;
	if (i >= len or data[i] != '(') return false;

	int depth = 0;
	bool quoted = false;
	for (; i < len; i++)
	{
		char c = data[i];
		if (quoted)
		{
			if (c == '\\') { i++; continue; }
			if (c == '"') quoted = false;
			continue;
		}
		if (c == '"') quoted = true;
		else if (c == '(') depth++;
		else if (c == ')')
		{
			if (0 == --depth) { i++; break; }
		}
	}
	if (0 != depth or quoted) return false;

	// Only trailing whitespace may follow the expression.
	while (i < len and std::isspace((unsigned char) data[i])) i++;
	return i == len;
}

/// The "new" control file: writing an Atomese s-expression (the same
/// format the sexpr shell speaks, e.g. `(Concept "foo")`) decodes it
/// and adds the atom to the AtomSpace; reading back then returns the
/// new atom's /by-handle path. Malformed or non-atom input makes the
/// write fail with an Rerror. One instance is shared by every
/// connection to the same AtomSpace, so the last-created path is a
/// server-wide convenience, not per-connection state.
class NewControlNode : public NineNode
{
public:
	explicit NewControlNode(AtomFsIndex* index) : _index(index) {}

	std::string name() const override { return "new"; }
	bool is_dir() const override { return false; }
	NineQid qid() const override
	{
		NineQid q; q.type = QTFILE; q.path = mix_id(0, "new-control");
		return q;
	}
	bool is_writable() const override
	{
		return not _index->atomspace()->get_read_only();
	}
	uint64_t length() const override
	{
		std::lock_guard<std::mutex> lock(_mtx);
		return _last_reply.size();
	}
	std::string read(uint64_t offset, uint32_t count) const override
	{
		std::lock_guard<std::mutex> lock(_mtx);
		if (offset >= _last_reply.size()) return std::string();
		return _last_reply.substr(offset, count);
	}
	uint32_t write(uint64_t offset, const std::string& data) override
	{
		(void) offset;
		if (not is_writable())
			throw std::runtime_error("new: read-only AtomSpace");

		// Require exactly one complete expression per write; the
		// decoder itself would accept unbalanced input, ignore
		// trailing text, and can crash on empty writes.
		if (not is_single_sexpr(data))
			throw std::runtime_error(
				"new: expecting exactly one s-expression");

		// Sexpr::decode_atom throws on malformed input; let that
		// propagate so the client sees an Rerror for the Twrite.
		// Use the explicit-cache overload with a local cache: the
		// implicit overload shares a process-wide static cache that
		// concurrent writers would race on.
		std::unordered_map<std::string, Handle> cache;
		size_t pos = 0;
		Handle h = Sexpr::decode_atom(data, pos, cache);
		if (nullptr == h)
			throw std::runtime_error("new: not an atom");
		h = _index->atomspace()->add_atom(h);
		if (nullptr == h)
			throw std::runtime_error("new: atom was not inserted");

		std::lock_guard<std::mutex> lock(_mtx);
		_last_reply = "/by-handle/" + AtomFsIndex::id_of(h) + "\n";
		return (uint32_t) data.size();
	}

private:
	AtomFsIndex* _index;
	mutable std::mutex _mtx;
	std::string _last_reply;
};

} // anonymous namespace

// ---------------------------------------------------------------
// AtomFsIndex: builds and memoizes the per-atom directory tree.

AtomFsIndex::AtomFsIndex(const AtomSpacePtr& as) :
	_as(as)
{
	AtomFsIndex* self = this;

	NineNodePtr by_handle(new FuncDirNode("by-handle", mix_id(0, "by-handle"),
		[self]() -> std::vector<NineNodePtr>
		{
			HandleSeq all;
			self->_as->get_handles_by_type(all, ATOM, true);
			std::vector<NineNodePtr> out;
			out.reserve(all.size());
			for (const Handle& h : all)
				out.push_back(self->dir_for(h));
			return out;
		},
		[self](const std::string& n) -> NineNodePtr
		{
			return self->atom_by_id(n, ATOM, true);
		}));

	NineNodePtr by_type(new FuncDirNode("by-type", mix_id(0, "by-type"),
		[self]() -> std::vector<NineNodePtr>
		{
			HandleSeq all;
			self->_as->get_handles_by_type(all, ATOM, true);
			std::vector<Type> seen;
			std::vector<NineNodePtr> out;
			for (const Handle& h : all)
			{
				Type t = h->get_type();
				bool dup = false;
				for (Type s : seen) if (s == t) { dup = true; break; }
				if (dup) continue;
				seen.push_back(t);
				out.push_back(self->type_dir(t));
			}
			return out;
		},
		[self](const std::string& n) -> NineNodePtr
		{
			Type t = nameserver().getType(n);
			if (NOTYPE == t) return nullptr;
			return self->type_dir(t);
		}));

	NineNodePtr new_ctl(new NewControlNode(self));

	_root = NineNodePtr(new FuncDirNode("/", mix_id(0, "root"),
		[by_type, by_handle, new_ctl]() -> std::vector<NineNodePtr>
		{
			return { by_type, by_handle, new_ctl };
		}));
}

NineNodePtr AtomFsIndex::type_dir(Type t)
{
	AtomFsIndex* self = this;
	const std::string tname = nameserver().getTypeName(t);
	return NineNodePtr(new FuncDirNode(
		tname, mix_id((uint64_t) t, "by-type-entry"),
		[self, t]() -> std::vector<NineNodePtr>
		{
			HandleSeq hs;
			self->_as->get_handles_by_type(hs, t, false);
			std::vector<NineNodePtr> kids;
			kids.reserve(hs.size());
			for (const Handle& h : hs)
				kids.push_back(self->dir_for(h));
			return kids;
		},
		[self, t](const std::string& n) -> NineNodePtr
		{
			return self->atom_by_id(n, t, false);
		}));
}

NineNodePtr AtomFsIndex::atom_by_id(const std::string& name,
                                    Type t, bool subclass)
{
	// Ids are exactly 16 lower-case hex digits (see id_of()).
	if (16 != name.size()) return nullptr;
	for (char c : name)
		if (not isxdigit((unsigned char) c)) return nullptr;
	ContentHash want =
		(ContentHash) std::strtoull(name.c_str(), nullptr, 16);

	// A cached node resolves the name without touching the AtomSpace.
	{
		std::lock_guard<std::mutex> lock(_mtx);
		auto it = _cache.find(want);
		if (it != _cache.end())
		{
			NineNodePtr live = it->second.lock();
			if (live) return live;
		}
	}

	// Scan for the matching atom; only the match gets a node built,
	// so a walk does not materialize the whole AtomSpace.
	HandleSeq hs;
	_as->get_handles_by_type(hs, t, subclass);
	for (const Handle& h : hs)
		if (h.value() == want)
			return dir_for(h);
	return nullptr;
}

NineNodePtr AtomFsIndex::dir_for(const Handle& h)
{
	std::lock_guard<std::mutex> lock(_mtx);

	ContentHash key = h.value();
	auto it = _cache.find(key);
	if (it != _cache.end())
	{
		NineNodePtr live = it->second.lock();
		if (live) return live;
		_cache.erase(it);
	}

	AtomFsIndex* self = this;
	Handle atom = h;
	const uint64_t qbase = key;
	const std::string id = id_of(h);

	NineNodePtr name_file(new TextFileNode("name", mix_id(qbase, "name"),
		[atom]() -> std::string
		{
			// get_name() throws for Links; they have no name.
			if (not atom->is_node()) return std::string();
			return atom->get_name() + "\n";
		}));

	NineNodePtr type_file(new TextFileNode("type", mix_id(qbase, "type"),
		[atom]() { return nameserver().getTypeName(atom->get_type()) + "\n"; }));

	NineNodePtr to_string_file(new TextFileNode("to_string",
		mix_id(qbase, "to_string"),
		[atom]() { return atom->to_string() + "\n"; }));

	NineNodePtr tv_file(new TvFileNode(_as, atom, mix_id(qbase, "tv")));

	NineNodePtr outgoing_dir(new FuncDirNode("outgoing",
		mix_id(qbase, "outgoing"),
		[self, atom]() -> std::vector<NineNodePtr>
		{
			std::vector<NineNodePtr> out;
			// getOutgoingSet() throws for Nodes; they have no
			// outgoing set, so their outgoing/ is simply empty.
			if (not atom->is_link()) return out;
			const HandleSeq& oset = atom->getOutgoingSet();
			out.reserve(oset.size());
			for (size_t i = 0; i < oset.size(); i++)
			{
				out.push_back(NineNodePtr(new AliasNode(
					std::to_string(i), self->dir_for(oset[i]))));
			}
			return out;
		}));

	NineNodePtr incoming_dir(new FuncDirNode("incoming",
		mix_id(qbase, "incoming"),
		[self, atom]() -> std::vector<NineNodePtr>
		{
			std::vector<NineNodePtr> out;
			IncomingSet iset = atom->getIncomingSet();
			out.reserve(iset.size());
			for (const Handle& ih : iset)
			{
				out.push_back(NineNodePtr(new AliasNode(
					AtomFsIndex::id_of(ih), self->dir_for(ih))));
			}
			return out;
		}));

	NineNodePtr values_dir(new FuncDirNode("values",
		mix_id(qbase, "values"),
		[atom, qbase]() -> std::vector<NineNodePtr>
		{
			std::vector<NineNodePtr> out;
			HandleSet keys = atom->getKeys();
			for (const Handle& key : keys)
			{
				std::string label = AtomFsIndex::id_of(key);
				// The qid must mix in the owning atom too: different
				// atoms holding a value under the same key are
				// different files, and must not share a qid. The
				// value itself is fetched fresh on every read, so an
				// open fid never serves stale data after the value
				// is replaced.
				out.push_back(NineNodePtr(new TextFileNode(label,
					mix_id(mix_id(qbase, "value-entry"), label),
					[atom, key]()
					{
						ValuePtr v = atom->getValue(key);
						return key->to_short_string() + " = " +
							(v ? v->to_string() : std::string("()")) + "\n";
					})));
			}
			return out;
		}));

	std::vector<NineNodePtr> kids =
		{ name_file, type_file, to_string_file, tv_file,
		  outgoing_dir, incoming_dir, values_dir };

	NineNodePtr dir(new FuncDirNode(id, qbase,
		[kids]() -> std::vector<NineNodePtr> { return kids; }));

	_cache[key] = dir;

	// The cache holds weak references (fids keep live nodes pinned);
	// sweep the dead entries occasionally so the map itself stays
	// proportional to the number of *live* nodes, not to every atom
	// ever visited.
	if (_cache.size() >= _sweep_mark)
	{
		for (auto i = _cache.begin(); i != _cache.end(); )
			i = i->second.expired() ? _cache.erase(i) : std::next(i);
		_sweep_mark = std::max<size_t>(64, 2 * _cache.size());
	}
	return dir;
}

} // namespace plan9
} // namespace opencog

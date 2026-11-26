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
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <unordered_map>

#include <opencog/atoms/atom_types/atom_types.h>
#include <opencog/atoms/atom_types/NameServer.h>
#include <opencog/atoms/base/Atom.h>
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
// Naming policy: how atoms appear as directory-entry names, and how
// such names (and file contents) are parsed back. Everything that
// names an atom or reads Atomese from a client goes through here.

/// Longest directory-entry name handed out: NAME_MAX on Linux,
/// whose v9fs client cannot look up anything longer. A stat entry
/// with a name this long must still fit in a directory read at the
/// smallest negotiable msize; see NINE_MIN_MSIZE.
static const size_t ATOMFS_NAME_MAX = 255;

/// True if `s` can serve as a directory-entry name on every client:
/// a slash would split it into two path elements, control
/// characters (NUL, newline, ...) cannot be carried or typed
/// reliably, and over-long names cannot be looked up.
static bool is_safe_name(const std::string& s)
{
	if (s.empty() or ATOMFS_NAME_MAX < s.size()) return false;
	for (unsigned char c : s)
		if (c == '/' or c < 0x20 or c == 0x7f) return false;
	return true;
}

/// Return true iff `data` holds exactly one nonempty, balanced
/// s-expression, optionally surrounded by whitespace. Parentheses
/// inside double-quoted strings (with backslash escapes) do not
/// count. Upstream Sexpr::decode_atom() is laxer than this: it can
/// crash on empty input, accepts unbalanced expressions, and
/// silently ignores trailing text, so client input is gated here.
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

/// Decode client-supplied text holding one Atomese s-expression
/// into an Atom, *without* adding it to any AtomSpace. Throws
/// NineError if the text is not exactly one well-formed atom.
static Handle parse_atom(const std::string& text)
{
	if (not is_single_sexpr(text))
		throw NineError(NINE_ERR_INVALID,
			"expecting exactly one s-expression");
	Handle h;
	try
	{
		// Use the explicit-cache overload with a local cache: the
		// implicit overload shares a process-wide static cache
		// that concurrent sessions would race on.
		std::unordered_map<std::string, Handle> cache;
		size_t pos = 0;
		h = Sexpr::decode_atom(text, pos, cache);
	}
	catch (const std::exception& e)
	{
		throw NineError(NINE_ERR_INVALID, e.what());
	}
	if (nullptr == h)
		throw NineError(NINE_ERR_INVALID, "not an atom");
	return h;
}

/// Decode client-supplied text holding one Atomese s-expression
/// into a Value (which may itself be an Atom). Throws NineError if
/// the text is not exactly one well-formed value.
static ValuePtr parse_value(const std::string& text)
{
	if (not is_single_sexpr(text))
		throw NineError(NINE_ERR_INVALID,
			"expecting exactly one s-expression");
	ValuePtr v;
	try
	{
		size_t pos = text.find('(');
		v = Sexpr::decode_value(text, pos);
	}
	catch (const std::exception& e)
	{
		throw NineError(NINE_ERR_INVALID, e.what());
	}
	if (nullptr == v)
		throw NineError(NINE_ERR_INVALID, "not a value");
	return v;
}

/// The canonical one-line spelling of an atom: full type names, no
/// line breaks, and accepted back by parse_atom(). Two atoms have
/// the same spelling iff they are the same atom.
static std::string spell(const Handle& h)
{
	return Sexpr::encode_atom(h);
}

std::string AtomFsIndex::name_of(const Handle& h)
{
	std::string s = spell(h);
	return is_safe_name(s) ? s : id_of(h);
}

Handle AtomFsIndex::atom_named(const std::string& name) const
{
	try
	{
		return _as->get_atom(parse_atom(name));
	}
	catch (const NineError&)
	{
		return Handle::UNDEFINED;
	}
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

/// A named alias for another node: same Qid, children, and
/// read/write behaviour as `_target`, but a different directory-entry
/// name. This is how the "hard links" are implemented: the exact
/// same underlying atom directory is reachable under several
/// different names (its id under by-handle/, its s-expression under
/// by-type/ and incoming/, a positional index under outgoing/), and
/// Qid equality (not object identity of the wrapper) is what makes
/// them the "same file".
///
/// Operations on the file itself are forwarded. remove() is not: it
/// acts on a directory entry, and unlinking one name of an atom is
/// not the same thing as removing the atom.
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
	bool is_writable() const override { return _target->is_writable(); }
	uint32_t write(uint64_t offset, const std::string& data) override
	{
		return _target->write(offset, data);
	}
	void truncate(uint64_t length) override { _target->truncate(length); }
	NineNodePtr create(const std::string& n, uint32_t perm) override
	{
		return _target->create(n, perm);
	}

private:
	std::string _name;
	NineNodePtr _target;
};

} // anonymous namespace

// ---------------------------------------------------------------
// Values: the (key . value) pairs attached to an atom.

/// The state shared by one atom's values/ directory and the files
/// in it. The index hands out one instance per atom (see
/// AtomFsIndex::dir_for()), which lives for as long as any node
/// refers to it.
struct ValueSlots
{
	ValueSlots(AtomSpacePtr a, Handle h, uint64_t q) :
		as(std::move(a)), atom(std::move(h)), qbase(q)
	{}

	const AtomSpacePtr as;
	const Handle atom;
	const uint64_t qbase;

	/// Keys created with Tcreate but not yet written, by spelling.
	/// A file must be reachable by name as soon as it is created
	/// -- a Linux client walks to it and stats it before sending
	/// the first Twrite -- but a key has no existence in the
	/// AtomSpace apart from its value. So the newborn, still empty
	/// file is remembered here, and nothing is put in the AtomSpace
	/// until a value is actually written. A file that is never
	/// written vanishes once nothing refers to these slots.
	std::mutex mtx;
	std::map<std::string, Handle> pending;

	/// The atom as currently visible in the exported space: the
	/// child-frame COW copy when one exists, otherwise the
	/// inherited original. Resolved afresh for every operation, so
	/// all views of the atom observe the same, shallowest copy, and
	/// nothing is cached across operations.
	Handle live() const
	{
		Handle cur = as->get_atom(atom);
		return cur ? cur : atom;
	}

	void require_writable() const
	{
		if (as->get_read_only())
			throw NineError(NINE_ERR_READ_ONLY);
	}
};

typedef std::shared_ptr<ValueSlots> ValueSlotsPtr;

namespace {

/// One value of one atom: a file named after the key, holding the
/// value as an s-expression, e.g.
///
///     values/(PredicateNode "*-TruthValueKey-*")
///     (FloatValue 0.5 0.25)
///
/// Writing an s-expression replaces the value; removing the file
/// deletes it. Changes go through AtomSpace::set_value(), so
/// changing an atom inherited from a read-only parent frame copies
/// it into the exported space (copy-on-write) instead of mutating
/// the parent. The value is fetched afresh on every read, so an
/// open fid never serves stale data after the value is replaced.
class ValueFileNode : public NineNode
{
public:
	ValueFileNode(ValueSlotsPtr slots, Handle key) :
		_slots(std::move(slots)), _key(std::move(key)),
		_spelling(spell(_key))
	{}

	std::string name() const override
	{
		return AtomFsIndex::name_of(_key);
	}
	bool is_dir() const override { return false; }
	NineQid qid() const override
	{
		// The qid mixes in the owning atom as well as the key:
		// different atoms holding a value under the same key are
		// different files, and must not share a qid.
		NineQid q;
		q.type = QTFILE;
		q.path = mix_id(mix_id(_slots->qbase, "value-entry"),
			AtomFsIndex::id_of(_key));
		return q;
	}
	uint64_t length() const override { return render().size(); }
	std::string read(uint64_t offset, uint32_t count) const override
	{
		std::string s = render();
		if (offset >= s.size()) return std::string();
		return s.substr(offset, count);
	}
	bool is_writable() const override
	{
		return not _slots->as->get_read_only();
	}

	uint32_t write(uint64_t offset, const std::string& data) override
	{
		_slots->require_writable();
		// The file holds a single value, which a write replaces
		// whole; there is no meaningful way to patch part of one.
		if (0 != offset)
			throw NineError(NINE_ERR_INVALID,
				"value: writes must be at offset 0");

		// Atoms inside the value belong in the AtomSpace, just as
		// they do when a value arrives from storage.
		AtomSpace* as = _slots->as.get();
		ValuePtr v = Sexpr::add_atoms(as, parse_value(data));

		// So does the key, when this write is what brings it into
		// being; a key that already holds a value is left as is.
		Handle key = is_pending() ? as->add_atom(_key) : _key;
		as->set_value(_slots->live(), key, v);
		drop_pending();
		return (uint32_t) data.size();
	}

	void truncate(uint64_t length) override
	{
		_slots->require_writable();
		// A value cannot be partly kept, so the only truncation
		// that makes sense is the one a replacing write performs
		// anyway. Truncating to nothing ahead of that write (as
		// O_TRUNC does) therefore has nothing to do. It
		// deliberately does not delete the value: a `>`
		// redirection whose write is then rejected must not lose
		// what was there.
		if (0 != length)
			throw NineError(NINE_ERR_TRUNCATE);
	}

	void remove() override
	{
		_slots->require_writable();
		bool existed = drop_pending();
		Handle atom = _slots->live();
		if (atom->getValue(_key))
		{
			_slots->as->set_value(atom, _key, nullptr);
			existed = true;
		}
		if (not existed)
			throw NineError(NINE_ERR_NOT_FOUND);
	}

private:
	bool is_pending() const
	{
		std::lock_guard<std::mutex> lock(_slots->mtx);
		return 0 < _slots->pending.count(_spelling);
	}
	bool drop_pending()
	{
		std::lock_guard<std::mutex> lock(_slots->mtx);
		return 0 < _slots->pending.erase(_spelling);
	}

	// A key that holds no value yet (a newly created file) reads
	// as empty.
	std::string render() const
	{
		ValuePtr v = _slots->live()->getValue(_key);
		if (nullptr == v) return std::string();
		return Sexpr::encode_value(v) + "\n";
	}

	ValueSlotsPtr _slots;
	Handle _key;
	std::string _spelling;
};

/// An atom's values/ directory: one ValueFileNode per key. Creating
/// a file here, named by the s-expression of the key, attaches a
/// new value to the atom.
class ValuesDirNode : public NineNode
{
public:
	explicit ValuesDirNode(ValueSlotsPtr slots) : _slots(std::move(slots)) {}

	std::string name() const override { return "values"; }
	bool is_dir() const override { return true; }
	NineQid qid() const override
	{
		NineQid q;
		q.type = QTDIR;
		q.path = mix_id(_slots->qbase, "values");
		return q;
	}
	bool is_writable() const override
	{
		return not _slots->as->get_read_only();
	}

	std::vector<NineNodePtr> children() const override
	{
		std::vector<NineNodePtr> out;
		for (const Handle& key : keys())
			out.push_back(std::make_shared<ValueFileNode>(_slots, key));
		return out;
	}

	/// A key answers to its listed name, to any other spelling of
	/// its s-expression (`(Predicate "k")` for `(PredicateNode
	/// "k")`), and to its id.
	NineNodePtr lookup(const std::string& child_name) const override
	{
		std::string want;
		try { want = spell(parse_atom(child_name)); }
		catch (const NineError&) {}

		for (const Handle& key : keys())
		{
			bool hit = want.empty()
				? (AtomFsIndex::id_of(key) == child_name)
				: (spell(key) == want);
			if (hit) return std::make_shared<ValueFileNode>(_slots, key);
		}
		return nullptr;
	}

	NineNodePtr create(const std::string& child_name,
	                   uint32_t perm) override
	{
		_slots->require_writable();
		if (perm & NINE_DMDIR)
			throw NineError(NINE_ERR_PERM,
				"values: cannot create a directory");

		// The file name says which key the value goes under.
		Handle key = parse_atom(child_name);
		std::string spelling = spell(key);

		// Checking that the name is free and reserving it are one
		// step: of two clients creating the same key at once,
		// exactly one must succeed.
		std::lock_guard<std::mutex> lock(_slots->mtx);
		for (const Handle& have : keys_locked())
			if (spell(have) == spelling)
				throw NineError(NINE_ERR_EXISTS);
		_slots->pending[spelling] = key;
		return std::make_shared<ValueFileNode>(_slots, key);
	}

private:
	/// Every key with a value, then every key created but not yet
	/// written.
	HandleSeq keys() const
	{
		std::lock_guard<std::mutex> lock(_slots->mtx);
		return keys_locked();
	}

	/// As keys(), for a caller that holds the slots' mutex.
	///
	/// The order of the two reads matters. The first write to a
	/// new file sets the value and only then drops the pending
	/// entry, so a key that is no longer pending here has its
	/// value by the time the values are read: a file never goes
	/// missing while it turns from pending into stored.
	HandleSeq keys_locked() const
	{
		HandleSeq out;
		std::set<std::string> waiting;
		for (const auto& p : _slots->pending)
			waiting.insert(p.first);

		for (const Handle& key : _slots->live()->getKeys())
		{
			waiting.erase(spell(key));
			out.push_back(key);
		}
		for (const std::string& spelling : waiting)
			out.push_back(_slots->pending.at(spelling));
		return out;
	}

	ValueSlotsPtr _slots;
};

// ---------------------------------------------------------------

/// The "new" control file: writing an Atomese s-expression (the same
/// format the sexpr shell speaks, e.g. `(Concept "foo")`) decodes it
/// and adds the atom to the AtomSpace; reading back then returns the
/// new atom's /by-handle path. The expression may carry values, as
/// in `(Concept "foo" (alist (cons (Predicate "k") (FloatValue 1))))`,
/// which are attached whether or not the atom already existed. Each
/// write must hold exactly one complete expression; anything else
/// makes the write fail. One
/// instance is shared by every connection to the same AtomSpace, so
/// the last-created path is a server-wide convenience, not
/// per-connection state.
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
		// Each write is one complete command, wherever the client
		// believes the file position to be (an appending `>>`
		// writes at the current length).
		(void) offset;
		require_writable();

		Handle decoded = parse_atom(data);
		AtomSpace* as = _index->atomspace().get();
		Handle h = as->add_atom(decoded);
		if (nullptr == h)
			throw NineError(NINE_ERR_IO, "new: atom was not inserted");

		// Values given along with the atom, as (alist (cons key
		// value) ...), are attached explicitly: adding an atom
		// that is already present would silently drop them. Their
		// keys, and any atoms they hold, join the AtomSpace too.
		for (const Handle& key : decoded->getKeys())
			h = as->set_value(h, as->add_atom(key),
				Sexpr::add_atoms(as, decoded->getValue(key)));

		std::lock_guard<std::mutex> lock(_mtx);
		_last_reply = "/by-handle/" + AtomFsIndex::id_of(h) + "\n";
		return (uint32_t) data.size();
	}
	void truncate(uint64_t length) override
	{
		// A control file stores nothing to cut short: the reply it
		// reads back is replaced by the next command. So opening
		// it with O_TRUNC, as `echo ... > new` does, is harmless.
		require_writable();
		if (0 != length)
			throw NineError(NINE_ERR_TRUNCATE);
	}

private:
	void require_writable() const
	{
		if (not is_writable())
			throw NineError(NINE_ERR_READ_ONLY);
	}

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
				kids.push_back(self->entry_for(h));
			return kids;
		},
		[self, t](const std::string& n) -> NineNodePtr
		{
			return self->atom_entry(n, t);
		}));
}

NineNodePtr AtomFsIndex::entry_for(const Handle& h)
{
	return NineNodePtr(new AliasNode(name_of(h), dir_for(h)));
}

NineNodePtr AtomFsIndex::atom_entry(const std::string& name, Type t)
{
	// The id always works, whatever name the atom is listed under.
	NineNodePtr by_id = atom_by_id(name, t, false);
	if (by_id) return by_id;

	Handle h = atom_named(name);
	if (nullptr == h or h->get_type() != t) return nullptr;
	return dir_for(h);
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

	// Mutable state (the values) must be read through slots->live(),
	// never through a handle captured at directory-build time. The
	// slots outlive this directory node whenever a file in values/
	// is still held open, and are then picked up again here.
	ValueSlotsPtr slots = _slots[key].lock();
	if (nullptr == slots)
	{
		slots = std::make_shared<ValueSlots>(_as, atom, qbase);
		_slots[key] = slots;
	}

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
		[slots]() { return slots->live()->to_string() + "\n"; }));

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
				out.push_back(self->entry_for(ih));
			return out;
		},
		[self, atom](const std::string& n) -> NineNodePtr
		{
			// As under by-type/: the listed name, any other
			// spelling of it, or the id. The name is compared with
			// the incoming links themselves rather than looked up
			// in the exported space, because an incoming set can
			// hold links that live in another frame.
			Handle named;
			try { named = parse_atom(n); }
			catch (const NineError&) {}

			for (const Handle& ih : atom->getIncomingSet())
			{
				bool hit = named ? (*ih == *named)
				                 : (AtomFsIndex::id_of(ih) == n);
				if (hit) return self->entry_for(ih);
			}
			return nullptr;
		}));

	NineNodePtr values_dir(new ValuesDirNode(slots));

	std::vector<NineNodePtr> kids =
		{ name_file, type_file, to_string_file,
		  outgoing_dir, incoming_dir, values_dir };

	NineNodePtr dir(new FuncDirNode(id, qbase,
		[kids]() -> std::vector<NineNodePtr> { return kids; }));

	_cache[key] = dir;

	// The caches hold weak references (fids keep live nodes pinned);
	// sweep the dead entries occasionally so the maps themselves stay
	// proportional to the number of *live* nodes, not to every atom
	// ever visited.
	if (_cache.size() >= _sweep_mark)
	{
		for (auto i = _cache.begin(); i != _cache.end(); )
			i = i->second.expired() ? _cache.erase(i) : std::next(i);
		for (auto i = _slots.begin(); i != _slots.end(); )
			i = i->second.expired() ? _slots.erase(i) : std::next(i);
		_sweep_mark = std::max<size_t>(64, 2 * _cache.size());
	}
	return dir;
}

} // namespace plan9
} // namespace opencog

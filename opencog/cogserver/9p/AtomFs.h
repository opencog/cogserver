/*
 * opencog/cogserver/9p/AtomFs.h
 *
 * Maps an AtomSpace onto a synthetic Plan 9 style file tree:
 *
 *   /by-type/<TYPE_NAME>/<atom>/        -- one directory per atom
 *   /by-handle/<id>/                    -- the same directories, by id
 *   /new                                -- control file: write an
 *                                          Atomese s-expression to
 *                                          create an atom, then read
 *                                          back its /by-handle path
 *
 * Each atom directory contains:
 *   name                                -- Node name (empty for Links)
 *   type                                -- type name
 *   to_string                           -- full to_string() rendering
 *   outgoing/0, outgoing/1, ...         -- hard-links to each element
 *                                          of the outgoing set, in order
 *   incoming/<atom>, ...                -- hard-links to each element
 *                                          of the incoming set
 *   values/<key>                        -- one file per (key . value)
 *                                          pair attached to the atom,
 *                                          named by the key and
 *                                          holding the value
 *
 * Names. `<atom>` and `<key>` are the atom's s-expression, written
 * on one line with full type names, e.g. `(ConceptNode "foo")` or
 * `(ListLink (ConceptNode "foo")(ConceptNode "bar"))` -- see
 * name_of(). `<id>` is the atom's content hash (Handle::value()),
 * as fixed-width hex -- see id_of(). An s-expression that cannot
 * serve as a file name (it contains a slash or a control character,
 * or exceeds 255 bytes) is listed under its id instead. Lookups are
 * more liberal than listings: wherever an atom is listed by its
 * s-expression, it also answers to its id and to any other spelling
 * that decodes to the same atom, such as `(Concept "foo")`.
 *
 * Values. A values/ file holds the value as an s-expression, e.g.
 * `(FloatValue 0.5 0.25)`, and the outgoing/ and values/
 * directories are otherwise alike: the first is positional and
 * anonymous, the second is keyed by name. Unless the AtomSpace is
 * read-only, writing an s-expression to a values/ file replaces the
 * value, creating a file named by a key attaches a new value, and
 * removing the file deletes it. Nothing else in an atom directory
 * can be changed.
 *
 * Every atom maps to exactly one, shared NineNode instance (see
 * AtomFsIndex::dir_for()), no matter how many different paths lead
 * to it -- by-type/, outgoing/ and incoming/ entries are the *same*
 * object as the corresponding by-handle/<id> entry, giving genuine
 * hard-link semantics (identical Qids) as in a real Plan 9
 * namespace.
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_ATOM_FS_H
#define _OPENCOG_ATOM_FS_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Handle.h>
#include <opencog/cogserver/9p/NineNode.h>

namespace opencog
{
namespace plan9
{
/** \addtogroup grp_server
 *  @{
 */

struct ValueSlots;

/**
 * Owns the mapping from an AtomSpace's atoms to their (memoized)
 * NineNode representations, and builds the top-level directory
 * tree (by-type/, by-handle/, new) for that AtomSpace.
 *
 * The per-atom node cache holds weak references: nodes stay alive
 * for exactly as long as something (a fid, a directory listing in
 * progress) references them, so the index never pins deleted atoms
 * and its memory use is bounded by the number of nodes actually in
 * use, not by the number of atoms ever visited. Qids are derived
 * deterministically from atom content hashes, so a rebuilt node is
 * indistinguishable from the original.
 *
 * An AtomFsIndex must outlive every NineNode it has handed out, and
 * outlive any NineSession built on top of its root().
 */
class AtomFsIndex
{
public:
	explicit AtomFsIndex(const AtomSpacePtr& as);

	/// The root directory node ("/") for this AtomSpace.
	NineNodePtr root() const { return _root; }

	/// Get-or-create the shared directory node for `h`.
	NineNodePtr dir_for(const Handle& h);

	const AtomSpacePtr& atomspace() const { return _as; }

	/// Stable, filename-safe identifier for a Handle: its content
	/// hash, rendered as fixed-width lower-case hex.
	static std::string id_of(const Handle& h);

	/// The name an atom is listed under: its one-line s-expression
	/// when that can serve as a file name, else its id_of().
	static std::string name_of(const Handle& h);

	/// The atom of this AtomSpace that `name` spells as an
	/// s-expression, or Handle::UNDEFINED if `name` is not a
	/// well-formed atom or no such atom exists. Never adds
	/// anything to the AtomSpace.
	Handle atom_named(const std::string& name) const;

private:
	/// Directory node for one atom type (a by-type/<TYPE_NAME>/ entry).
	NineNodePtr type_dir(Type t);

	/// The directory entry for `h` wherever atoms are listed by
	/// name: its shared directory node, under its name_of().
	NineNodePtr entry_for(const Handle& h);

	/// Resolve one path component, naming an atom of exactly type
	/// `t` by id or by s-expression, to its node; nullptr if there
	/// is no such atom.
	NineNodePtr atom_entry(const std::string& name, Type t);

	/// Resolve one <id> path component directly to its atom's node,
	/// searching atoms of type `t` (with subtypes if `subclass`);
	/// nullptr if the name is not a valid id or no atom matches.
	NineNodePtr atom_by_id(const std::string& name, Type t, bool subclass);

	AtomSpacePtr _as;
	std::mutex _mtx;
	std::unordered_map<ContentHash, std::weak_ptr<NineNode>> _cache;
	std::unordered_map<ContentHash, std::weak_ptr<ValueSlots>> _slots;
	size_t _sweep_mark = 64;
	NineNodePtr _root;
};

typedef std::shared_ptr<AtomFsIndex> AtomFsIndexPtr;

/** @}*/
} // namespace plan9
} // namespace opencog

#endif // _OPENCOG_ATOM_FS_H

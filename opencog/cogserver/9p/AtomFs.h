/*
 * opencog/cogserver/9p/AtomFs.h
 *
 * Maps an AtomSpace onto a synthetic Plan 9 style file tree:
 *
 *   /by-type/<TYPE_NAME>/<id>/          -- one directory per atom
 *   /by-handle/<id>/                    -- the same directories, by id
 *   /new                                -- control file: write an
 *                                          Atomese s-expression to
 *                                          create an atom, then read
 *                                          back its /by-handle path
 *
 * Per atom directory <id>/ contains:
 *   name                                -- Node name (empty for Links)
 *   type                                -- type name
 *   to_string                           -- full to_string() rendering
 *   tv                                  -- convenience view of the
 *                                          truth-value key, if any
 *   outgoing/0, outgoing/1, ...         -- hard-links to each element
 *                                          of the outgoing set, in order
 *   incoming/<id>, ...                  -- hard-links to each element
 *                                          of the incoming set
 *   values/<id>                         -- one file per (key . value)
 *                                          pair attached to the atom
 *
 * `<id>` is the atom's content hash (Handle::value()), rendered as a
 * fixed-width hex string, so it is stable for a given atom's content
 * and safe to use as a path component.
 *
 * Every atom maps to exactly one, shared NineNode instance (see
 * AtomFsIndex::dir_for()), no matter how many different paths lead
 * to it -- outgoing/incoming "links" are the *same* object as the
 * corresponding by-handle/<id> entry, giving genuine hard-link
 * semantics (identical Qids) as in a real Plan 9 namespace.
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

/**
 * Owns the mapping from an AtomSpace's atoms to their (memoized)
 * NineNode representations, and builds the top-level directory
 * tree (by-type/, by-handle/, new) for that AtomSpace.
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

private:
	AtomSpacePtr _as;
	std::mutex _mtx;
	std::unordered_map<ContentHash, NineNodePtr> _cache;
	NineNodePtr _root;
};

typedef std::shared_ptr<AtomFsIndex> AtomFsIndexPtr;

/** @}*/
} // namespace plan9
} // namespace opencog

#endif // _OPENCOG_ATOM_FS_H

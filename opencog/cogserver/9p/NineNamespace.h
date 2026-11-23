/*
 * opencog/cogserver/9p/NineNamespace.h
 *
 * A per-connection Plan 9 style namespace: a table mapping namespace
 * paths to ordered stacks of NineNode trees ("union directories"),
 * built up via bind()/mount(), and resolved with walk(). See
 * Pike & Presotto et al., "Plan 9 from Bell Labs", and the bind(1)
 * / mount(1) / intro(5) manual pages for the semantics this models.
 *
 * Each 9P connection (see NineSession) owns an independent
 * NineNamespace, typically obtained by copy-constructing from a
 * shared base namespace -- mirroring Plan 9's rfork(RFNAMEG): the
 * copy starts with the same view, but subsequent binds in either the
 * original or the copy do not affect the other.
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_NINE_NAMESPACE_H
#define _OPENCOG_NINE_NAMESPACE_H

#include <map>
#include <string>
#include <vector>

#include <opencog/cogserver/9p/NineNode.h>

namespace opencog
{
namespace plan9
{
/** \addtogroup grp_server
 *  @{
 */

/// Union-directory ordering, as in Plan 9's bind(1)/mount(1) -M/-b/-a
/// flags: MREPL replaces whatever was there, MBEFORE puts the new
/// tree earlier (shadowing it), MAFTER puts it later (shadowed by it).
enum NineMountOrder { MREPL, MBEFORE, MAFTER };

/// One directory entry as seen through the namespace: the name the
/// entry has in its containing directory, plus the node that should
/// supply its metadata/contents (which, under a mount/bind override,
/// may be a differently-named node from another tree).
struct NineDirEnt
{
	std::string name;
	NineNodePtr node;
};

class NineNamespace
{
public:
	explicit NineNamespace(NineNodePtr root);
	NineNamespace(const NineNamespace&) = default;
	NineNamespace& operator=(const NineNamespace&) = default;

	/// Attach `device` at `at_path`, unioned per `order` with
	/// whatever is already visible there.
	void mount(const std::string& at_path, NineNodePtr device,
	           NineMountOrder order = MREPL);

	/// Bind whatever currently resolves at `old_path` onto
	/// `new_path` as well (Plan 9 `bind old_path new_path`).
	/// Throws std::runtime_error if `old_path` does not resolve.
	void bind(const std::string& new_path, const std::string& old_path,
	          NineMountOrder order = MREPL);

	/// Remove any override previously installed (by mount()/bind())
	/// at exactly `at_path`, reverting it to whatever the underlying
	/// static tree provides.
	void unmount(const std::string& at_path);

	/// Resolve `path` to the node that should be used for
	/// non-directory operations: the first (shadowing) entry of the
	/// union stack, or nullptr if `path` does not resolve.
	NineNodePtr walk(const std::string& path) const;

	/// Resolve `path` to its full union stack (outermost/shadowing
	/// entry first), for directory-listing purposes. Empty if `path`
	/// does not resolve at all. Union stacks exist only at explicit
	/// mount points: descending through an ordinary name keeps only
	/// the first (shadowing) tree that provides it, so entries
	/// shadowed at one level cannot reappear deeper down.
	std::vector<NineNodePtr> walk_union(const std::string& path) const;

	/// The union of all (name-deduplicated, first-occurrence-wins)
	/// children visible at `path`, honoring the same shadow order as
	/// walk_union(). Each child with a mount/bind override at its own
	/// path is listed under its original name but with the overriding
	/// node, so listed metadata agrees with walk().
	std::vector<NineDirEnt> list(const std::string& path) const;

	/// As list(), but over an already-resolved union stack for the
	/// canonical directory path `path` -- used to read a directory
	/// through the view captured when it was walked (so later
	/// mounts/unmounts at that path do not retarget it), while still
	/// resolving per-child overrides through the current mount table.
	std::vector<NineDirEnt> list_union(const std::string& path,
	                     const std::vector<NineNodePtr>& stack) const;

	/// Canonicalize a path: collapse repeated/trailing slashes,
	/// always returning a leading "/" (root is "/").
	static std::string canon(const std::string& path);

	/// Split a path into its non-empty components.
	static std::vector<std::string> split(const std::string& path);

private:
	// Keyed by canonical path; each entry is the union stack that
	// should be used starting at (and below, until another override
	// is met) that exact path.
	std::map<std::string, std::vector<NineNodePtr>> _mounts;
};

/** @}*/
} // namespace plan9
} // namespace opencog

#endif // _OPENCOG_NINE_NAMESPACE_H

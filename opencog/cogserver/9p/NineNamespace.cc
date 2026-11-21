/*
 * opencog/cogserver/9p/NineNamespace.cc
 *
 * Copyright (c) 2024 OpenCog Foundation
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/cogserver/9p/NineNamespace.h>

#include <stdexcept>
#include <unordered_set>

namespace opencog
{
namespace plan9
{

std::vector<std::string> NineNamespace::split(const std::string& path)
{
	std::vector<std::string> out;
	std::string cur;
	for (char c : path)
	{
		if (c == '/')
		{
			if (not cur.empty()) { out.push_back(cur); cur.clear(); }
		}
		else cur.push_back(c);
	}
	if (not cur.empty()) out.push_back(cur);
	return out;
}

std::string NineNamespace::canon(const std::string& path)
{
	std::vector<std::string> comps = split(path);
	if (comps.empty()) return "/";
	std::string out;
	for (const auto& c : comps) { out += "/"; out += c; }
	return out;
}

NineNamespace::NineNamespace(NineNodePtr root)
{
	_mounts["/"] = { std::move(root) };
}

std::vector<NineNodePtr> NineNamespace::walk_union(const std::string& path) const
{
	std::vector<std::string> comps = split(path);

	auto root_it = _mounts.find("/");
	std::vector<NineNodePtr> stack =
		(root_it != _mounts.end()) ? root_it->second : std::vector<NineNodePtr>();
	if (stack.empty())
		return {};

	std::string so_far;
	for (const auto& c : comps)
	{
		so_far += "/";
		so_far += c;

		// An explicit mount/bind at this exact path overrides
		// whatever the underlying trees provide -- and lets a bind
		// create a fresh path that no underlying tree contains.
		auto ov = _mounts.find(so_far);
		if (ov != _mounts.end() and not ov->second.empty())
		{
			stack = ov->second;
			continue;
		}

		// Union semantics apply only at explicit mount points: when
		// descending through an ordinary name, the first tree in the
		// stack that provides it wins, and the walk continues in that
		// single tree. Keeping every same-named child would let an
		// entry shadowed here become visible again one level down.
		NineNodePtr child;
		for (const auto& n : stack)
		{
			if (not n or not n->is_dir()) continue;
			child = n->lookup(c);
			if (child) break;
		}
		if (not child)
			return {};
		stack = { std::move(child) };
	}
	return stack;
}

NineNodePtr NineNamespace::walk(const std::string& path) const
{
	std::vector<NineNodePtr> stack = walk_union(path);
	return stack.empty() ? nullptr : stack.front();
}

std::vector<NineDirEnt> NineNamespace::list(const std::string& path) const
{
	return list_union(canon(path), walk_union(path));
}

std::vector<NineDirEnt> NineNamespace::list_union(const std::string& path,
                              const std::vector<NineNodePtr>& stack) const
{
	std::vector<NineDirEnt> out;
	std::unordered_set<std::string> seen;

	for (const auto& n : stack)
	{
		if (not n or not n->is_dir()) continue;
		for (const auto& child : n->children())
		{
			if (not child) continue;
			std::string name = child->name();
			if (not seen.insert(name).second) continue;

			// A mount/bind at the child's own path overrides the
			// underlying entry, so the listed metadata matches what
			// walk() would resolve there. The directory-entry name
			// stays the underlying one, even if the mounted node is
			// named differently.
			NineNodePtr shown = child;
			std::string childpath =
				(path == "/") ? "/" + name : path + "/" + name;
			auto ov = _mounts.find(childpath);
			if (ov != _mounts.end() and not ov->second.empty()
			    and ov->second.front())
				shown = ov->second.front();

			out.push_back(NineDirEnt{ std::move(name), std::move(shown) });
		}
	}
	return out;
}

void NineNamespace::mount(const std::string& at_path, NineNodePtr device,
                          NineMountOrder order)
{
	std::string canonical = canon(at_path);

	std::vector<NineNodePtr> stack;
	auto it = _mounts.find(canonical);
	if (it != _mounts.end())
		stack = it->second;
	else
		stack = walk_union(canonical);

	switch (order)
	{
		case MREPL:
			stack = { std::move(device) };
			break;
		case MBEFORE:
			stack.insert(stack.begin(), std::move(device));
			break;
		case MAFTER:
			stack.push_back(std::move(device));
			break;
	}
	_mounts[canonical] = std::move(stack);
}

void NineNamespace::bind(const std::string& new_path,
                         const std::string& old_path, NineMountOrder order)
{
	std::vector<NineNodePtr> src = walk_union(canon(old_path));
	if (src.empty())
		throw std::runtime_error("bind: source path not found: " + old_path);

	std::string canonical = canon(new_path);
	std::vector<NineNodePtr> stack;
	auto it = _mounts.find(canonical);
	if (it != _mounts.end())
		stack = it->second;
	else
		stack = walk_union(canonical); // fine if empty: new_path may be fresh

	switch (order)
	{
		case MREPL:
			stack = src;
			break;
		case MBEFORE:
			stack.insert(stack.begin(), src.begin(), src.end());
			break;
		case MAFTER:
			stack.insert(stack.end(), src.begin(), src.end());
			break;
	}
	_mounts[canonical] = std::move(stack);
}

void NineNamespace::unmount(const std::string& at_path)
{
	std::string canonical = canon(at_path);
	if (canonical == "/")
		throw std::runtime_error("unmount: cannot unmount the root");
	_mounts.erase(canonical);
}

} // namespace plan9
} // namespace opencog

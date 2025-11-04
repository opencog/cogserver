/*
 * McpPlugAtomSpace.cc
 *
 * MCP Plugin for AtomSpace operations
 * Copyright (c) 2025 Linas Vepstas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "McpPlugAtomSpace.h"
#include <opencog/persist/json/JSCommands.h>

using namespace opencog;

// Helper to add a tool description
static void add_tool(std::string& json, const std::string& name, 
                     const std::string& description,
                     const std::string& schema,
                     bool last = false)
{
	json += "\t{\n";
	json += "\t\t\"name\": \"" + name + "\",\n";
	json += "\t\t\"description\": \"" + description + "\",\n";
	json += "\t\t\"inputSchema\": " + schema + "\n";
	json += "\t}";
	if (!last) json += ",";
	json += "\n";
}

// Tool descriptions for all AtomSpace operations
std::string McpPlugAtomSpace::get_tool_descriptions() const
{
	std::string json = "[\n";

	// version
	add_tool(json, "version", "Get the AtomSpace version string",
		"{\"type\": \"object\", \"properties\": {}, \"required\": []}");

	// getSubTypes
	add_tool(json, "getSubTypes", "Get all subtypes of a given atom type. Useful for exploring the type hierarchy.",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to get subtypes of. Examples: 'Node', 'Link', 'Value'. Returns all types that inherit from this type.\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"If true, gets all descendants recursively. If false (default), gets only immediate children.\"}}, "
		"\"required\": [\"type\"]}");

	// getSuperTypes
	add_tool(json, "getSuperTypes", "Get all supertypes of a given atom type. Useful for exploring the type hierarchy.",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to get supertypes of. Examples: 'Concept', 'Edge', 'FloatValue'. Returns all types this type inherits from.\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"If true, gets all ancestors recursively up to TopType. If false (default), gets only immediate parents.\"}}, "
		"\"required\": [\"type\"]}");

	// reportCounts
	add_tool(json, "reportCounts", "A report of how many Atoms there are in the AtomSpace, organized by Atom type.",
		"{\"type\": \"object\", \"properties\": {}, \"required\": []}");

	// getAtoms
	add_tool(json, "getAtoms", "Get all atoms of a specific type from the AtomSpace. WARNING: May return large results - check count with reportCounts first.",
		"{\"type\": \"object\", \"properties\": {"
		"\"type\": {\"type\": \"string\", \"description\": \"The atom type to retrieve. Examples: 'Concept', 'Predicate', 'Edge', 'List'. Use getSubTypes/getSuperTypes to explore type hierarchy.\"}, "
		"\"subclass\": {\"type\": \"boolean\", \"description\": \"Whether to include atoms of subtypes. If true, retrieves all subtypes of the given type.\"}}, "
		"\"required\": [\"type\"]}");

	// haveNode
	add_tool(json, "haveNode", "Check if a node exists in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the node, e.g. (Concept \\\"cat\\\")\"}}, "
		"\"required\": [\"atomese\"]}");

	// haveLink
	add_tool(json, "haveLink", "Check if a link exists in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the link, e.g. (List (Concept \\\"a\\\") (Concept \\\"b\\\"))\"}}, "
		"\"required\": [\"atomese\"]}");

	// haveAtom
	add_tool(json, "haveAtom", "Check if an atom exists in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom\"}}, "
		"\"required\": [\"atomese\"]}");

	// makeAtom
	add_tool(json, "makeAtom", "Create an atom in the AtomSpace",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom to create. Examples: (Concept \\\"cat\\\"), (List (Concept \\\"a\\\") (Concept \\\"b\\\")), (Edge (Predicate \\\"likes\\\") (List (Concept \\\"Alice\\\") (Concept \\\"Bob\\\"))). Nodes and Links can be arbitrarily nested.\"}}, "
		"\"required\": [\"atomese\"]}");

	// getIncoming
	add_tool(json, "getIncoming", "Get all links that contain a given atom in their outgoing set",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom\"}}, "
		"\"required\": [\"atomese\"]}");

	// getKeys
	add_tool(json, "getKeys", "Get all keys attached to an atom",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom\"}}, "
		"\"required\": [\"atomese\"]}");

	// getValueAtKey
	add_tool(json, "getValueAtKey", "Get the value on an atom located at a given key. Returns a Value (FloatValue, StringValue, LinkValue, or Atom) in s-expression format.",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom. Example: (Anchor \\\"my-data\\\")\"}, "
		"\"key\": {\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the key atom. Example: (Predicate \\\"my-key\\\")\"}}, \"required\": [\"atomese\"]}}, "
		"\"required\": [\"atomese\", \"key\"]}");

	// getValues
	add_tool(json, "getValues", "Get all values attached to an atom. Returns an association list (alist) of (key . value) pairs in s-expression format.",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom. Example: (Anchor \\\"my-data\\\")\"}}, "
		"\"required\": [\"atomese\"]}");

	// setValue
	add_tool(json, "setValue", "Set a value on an atom with a given key",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom. Example: (Anchor \\\"my-data\\\")\"}, "
		"\"key\": {\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the key atom. Example: (Predicate \\\"my-key\\\")\"}}, \"required\": [\"atomese\"]}, "
		"\"value\": {\"type\": \"object\", \"description\": \"The value to set. PREFERRED: Use 'atomese' property with s-expression. Examples: {\\\"atomese\\\": \\\"(FloatValue 1.5 2.7 3.14)\\\"}, {\\\"atomese\\\": \\\"(StringValue \\\\\\\"hello\\\\\\\" \\\\\\\"world\\\\\\\")\\\"}, {\\\"atomese\\\": \\\"(LinkValue (Concept \\\\\\\"A\\\\\\\") (Concept \\\\\\\"B\\\\\\\"))\\\"}. Alternative: verbose JSON format {\\\"type\\\": \\\"FloatValue\\\", \\\"value\\\": [1.5, 2.7, 3.14]}.\", \"properties\": {\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the value (PREFERRED). Examples: (FloatValue 1.0 2.0), (StringValue \\\\\\\"text\\\\\\\"), (LinkValue (Concept \\\\\\\"X\\\\\\\") (Concept \\\\\\\"Y\\\\\\\"))\"}, \"type\": {\"type\": \"string\", \"description\": \"Type name for verbose JSON (not recommended)\"}, \"value\": {\"description\": \"Value data for verbose JSON (not recommended)\"}}}}, "
		"\"required\": [\"atomese\", \"key\", \"value\"]}");

	// execute
	add_tool(json, "execute", "Execute an executable atom and get the result. WARNING: Execution has side effects and may modify AtomSpace contents or external systems. Returns a Value.",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the executable atom. Examples: (PlusLink (Number 1) (Number 2)), (ValueOf (Concept \\\"foo\\\") (Predicate \\\"key\\\")), (Query ...). Many Link types are executable - see wiki for details.\"}}, "
		"\"required\": [\"atomese\"]}");

	// extract
	add_tool(json, "extract", "Remove an atom from the AtomSpace. WARNING: Irreversible operation.",
		"{\"type\": \"object\", \"properties\": {"
		"\"atomese\": {\"type\": \"string\", \"description\": \"S-expression for the atom to remove. Example: (Concept \\\"obsolete\\\")\"}, "
		"\"recursive\": {\"type\": \"boolean\", \"description\": \"If true, recursively removes all Links containing this atom. If false (default), only removes the atom if nothing references it.\"}}, "
		"\"required\": [\"atomese\"]}", true);  // last = true

	json += "]";
	return json;
}

std::string McpPlugAtomSpace::invoke_tool(const std::string& tool_name,
                                          const std::string& arguments) const
{
	// Construct the MCP command format that JSCommands expects
	std::string mcp_command = "{ \"tool\": \"" + tool_name + "\", \"params\": " + arguments + "}";

	// Use JSCommands to process the command
	return JSCommands::interpret_command(_as, mcp_command);
}

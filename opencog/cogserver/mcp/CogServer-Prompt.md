MCP Server Access via CogServer
================================

## What is the CogServer?

The CogServer is a network server application that provides access to the AtomSpace hypergraph database. It manages the in-RAM AtomSpace and exposes it through multiple network protocols, allowing clients to connect and interact with Atomese data structures.

## MCP Server Provided by CogServer

Your Atomese MCP tools (mcp__atomese__makeAtom, mcp__atomese__execute, mcp__atomese__setValue, etc.) are provided by the CogServer's MCP (Model Context Protocol) implementation. The CogServer exposes the same AtomSpace through multiple network interfaces:

* **MCP Server** - Model Context Protocol for LLM tool integration
  - Raw TCP/IP: port 18888 (default, most performant)
  - HTTP: port 18080 at path `/mcp`
  - WebSocket: port 18080 at path `/mcp`
* **Telnet Server** (port 17001, default) - Interactive command-line interface for direct Scheme/Atomese evaluation
* **HTTP/WebSocket Server** (port 18080, default) - Web-based interface for monitoring and visualization

All three ports access the same shared AtomSpace instance, so changes made through one interface are immediately visible through the others.

## MCP Documentation Resources

The MCP server documentation files are located in the cogserver source tree at:

**cogserver/opencog/cogserver/mcp/**

This directory contains:
* AtomSpace-Overview.md - Introduction to AtomSpace concepts
* AtomSpace-Details.md - Comprehensive guide to Atomese and the AtomSpace
* AtomTypes-Prompt.md - Reference for 170+ Atom types
* CreateAtom-Prompt.md - Guide for creating Nodes and Links
* DesigningStructures-Prompt.md - Guide for designing data structures
* PatternMatching-Prompt.md - Guide for pattern matching queries
* QueryAtom-Prompt.md - Guide for querying the AtomSpace
* WorkingWithValues-Prompt.md - Guide for working with Values
* AdvancedPatternMatching-Prompt.md - Advanced pattern matching techniques

These files are installed to **/usr/local/share/cogserver/mcp/** during installation.

## Important Usage Notes

When using MCP tools with Atomese s-expressions, keep these critical guidelines in mind:

* **Write all Atomese on a single line** - The atomese parameter is a JSON string field
* **Do NOT use `\n` in parameter strings** - This creates invalid JSON that causes the backslash to be stripped, leaving just 'n'
* **Atomese handles newlines fine** - The issue is not with Atomese, but with creating invalid JSON in MCP tool calls

### The Problem Explained

When you attempt to use `\n` (backslash-n) in MCP tool parameter strings, you are creating invalid JSON. The standard JSON library (libjsoncpp-dev) processes this incorrectly, stripping the backslash and leaving just the 'n' character. This 'n' then gets concatenated with the preceding text, corrupting atom type names.

For example, if you write:

```
mcp__atomese__makeAtom({
  "atomese": "(FilterLink\n  (Rule ...))"
})
```

The backslash is stripped, leaving just 'n', which gets concatenated: "FilterLink" + "n" = "FilterLinkn". This causes the error: "unknown Atom type: FilterLinkn".

**Note:** Atomese itself has no problem with newlines. The issue is specifically with how `\n` is handled when creating JSON strings for MCP tool parameters.

### Correct Usage Example

Write all Atomese on a single line without attempting to use escape sequences:

```
mcp__atomese__makeAtom({
  "atomese": "(FilterLink (Rule (TypedVariable (Variable \"$txt\") (Type 'StringStream)) (Variable \"$txt\") (LgParseBonds (Variable \"$txt\") (LgDict \"en\") (Number 1))) (ValueOf (Anchor \"tool-guide-parse\") (Predicate \"text-source\")))"
})
```

### Incorrect Usage Example

Do not attempt to use `\n` for formatting:

```
mcp__atomese__makeAtom({
  "atomese": "(FilterLink\n  (Rule ...)\n  (ValueOf ...))"
})
```

This creates invalid JSON and will fail with atom type name corruption errors.

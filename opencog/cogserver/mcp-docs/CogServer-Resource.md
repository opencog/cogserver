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

### Connection Behavior

**TCP/IP Port (18888):**
* Creates a persistent connection that remains open until explicitly closed
* The server sends JSON-RPC responses but does not close the connection
* When using tools like `netcat` or `telnet`, you must manually close the connection (Ctrl-C or Ctrl-D)
* Each JSON-RPC request gets a response, but the socket remains active for additional requests
* This allows multiple requests over a single connection for better performance

**HTTP Port (18080):**
* Standard HTTP request/response cycle - connection closes after each request
* Use POST to `/mcp` endpoint with JSON-RPC payload
* More suitable for one-off requests or web-based clients

**WebSocket Port (18080):**
* Persistent bidirectional connection like raw TCP
* Use `/mcp` WebSocket endpoint
* Connection remains open for streaming requests/responses

## MCP Documentation

The CogServer provides comprehensive documentation through MCP resources that are always available for reading.

### MCP Resources (Always-Available Documentation)

Access via `resources/read` with these URIs:

* `atomspace://docs/introduction` - Introduction to AtomSpace concepts
* `atomspace://docs/atomspace-guide` - Comprehensive guide to Atomese and the AtomSpace
* `atomspace://docs/cogserver-mcp` - CogServer and MCP tools guide (this document)
* `atomspace://docs/atom-types` - Comprehensive reference for 170+ Atom types organized by functional category
* `atomspace://docs/create-atom` - Guide for creating Nodes and Links in the AtomSpace
* `atomspace://docs/designing-structures` - Guide for designing data structures in Atomese
* `atomspace://docs/query-atom` - Guide for querying and exploring the AtomSpace effectively
* `atomspace://docs/working-with-values` - Guide for working with Values and key-value pairs
* `atomspace://docs/pattern-matching` - Guide for using MeetLink and QueryLink to search the AtomSpace
* `atomspace://docs/advanced-pattern-matching` - Guide for using AbsentLink, ChoiceLink, AlwaysLink, and GroupLink

### MCP Prompts (Alternative Access for Context Injection)

The same documentation is also available via `prompts/get` for user-initiated context injection:

* `atom-types-reference`, `create-atoms`, `designing-structures`, `query-atomspace`, `work-with-values`, `pattern-matching`, `advanced-pattern-matching`

**Note:** LLM clients should read the resources proactively to build comprehensive knowledge. Prompts are primarily for user-directed context injection.

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

## MCP Tool Usage Patterns

### Using the "atomese" Field

Most MCP tools accept an `"atomese"` field in their parameters, allowing you to write natural Atomese s-expressions. This field tunnels s-expressions directly to the Atomese parser, providing the cleanest syntax.

**Key MCP Tools:**
* `makeAtom` - Create Atoms (Nodes and Links)
* `setValue` - Attach Values to Atoms
* `execute` - Execute executable Atoms
* `haveAtom` / `haveNode` / `haveLink` - Check existence
* `getValueAtKey` / `getValues` - Retrieve Values
* `getIncoming` - Get Links containing an Atom

### Working with Values: The setValue Tool

The `setValue` tool attaches Values to Atoms using a key-value database. **CRITICAL:** The `value` parameter accepts an `"atomese"` property for natural s-expression syntax.

**setValue Parameters:**
* `atomese` - S-expression for the target Atom
* `key` - Object with `atomese` property for the key Atom (usually a PredicateNode)
* `value` - Object with `atomese` property for the Value to attach

**PREFERRED: Use the "atomese" field in the value parameter:**

```javascript
mcp__atomese__setValue({
  "atomese": "(Anchor \"my-data\")",
  "key": {"atomese": "(Predicate \"my-key\")"},
  "value": {"atomese": "(FloatValue 1.5 2.7 3.14)"}
})
```

**Examples of Value Types:**

```javascript
// FloatValue - Vector of floating-point numbers
{"atomese": "(FloatValue 1.5 2.7 3.14)"}

// StringValue - Vector of strings
{"atomese": "(StringValue \"hello\" \"world\")"}

// LinkValue - Vector of Values (can contain Atoms or other Values)
{"atomese": "(LinkValue (Concept \"A\") (Concept \"B\") (Concept \"C\"))"}

// Mixed LinkValue - Atoms and Values together
{"atomese": "(LinkValue (Concept \"X\") (FloatValue 1.0 2.0) (StringValue \"text\"))"}

// Atom as Value - Since Atoms are Values, can use directly
{"atomese": "(Concept \"SomeAtom\")"}
```

**Complete setValue Example:**

```javascript
// Create anchor point
mcp__atomese__makeAtom({"atomese": "(Anchor \"experimental-data\")"})

// Attach float values
mcp__atomese__setValue({
  "atomese": "(Anchor \"experimental-data\")",
  "key": {"atomese": "(Predicate \"measurements\")"},
  "value": {"atomese": "(FloatValue 23.5 24.1 23.8 24.3)"}
})

// Retrieve the values
mcp__atomese__getValueAtKey({
  "atomese": "(Anchor \"experimental-data\")",
  "key": {"atomese": "(Predicate \"measurements\")"}
})
// Returns: (FloatValue 23.5 24.1 23.8 24.3)
```

**Alternative: Verbose JSON Format (not recommended)**

You can also use pure JSON format, but it's more verbose:

```javascript
{
  "atomese": "(Anchor \"data\")",
  "key": {"atomese": "(Predicate \"key\")"},
  "value": {
    "type": "FloatValue",
    "value": [1.5, 2.7, 3.14]
  }
}
```

The "atomese" field approach is strongly preferred for its clarity and natural s-expression syntax.

### Common Patterns

**Pattern 1: Store and retrieve data**
```javascript
// Store
mcp__atomese__setValue({
  "atomese": "(Anchor \"pipeline-results\")",
  "key": {"atomese": "(Predicate \"status\")"},
  "value": {"atomese": "(StringValue \"completed\" \"2025-11-03\")"}
})

// Retrieve
mcp__atomese__getValueAtKey({
  "atomese": "(Anchor \"pipeline-results\")",
  "key": {"atomese": "(Predicate \"status\")"}
})
```

**Pattern 2: Check existence before creating**
```javascript
// Check if atom exists
mcp__atomese__haveAtom({"atomese": "(Concept \"cat\")"})
// Returns: true or false

// Create only if needed
if (!exists) {
  mcp__atomese__makeAtom({"atomese": "(Concept \"cat\")"})
}
```

**Pattern 3: Execute and retrieve results**
```javascript
// Execute computation
mcp__atomese__execute({
  "atomese": "(PlusLink (Number 10) (Number 32))"
})
// Returns: (NumberNode "42")
```

### For More Information

* **Atomese basics** - See `atomspace://docs/atomspace-guide`
* **Value types and usage** - See `atomspace://docs/working-with-values`
* **Pattern matching** - See `atomspace://docs/pattern-matching` and `atomspace://docs/query-atom`
* **Advanced patterns** - See `atomspace://docs/advanced-pattern-matching`
* **Atom types** - See `atomspace://docs/atom-types` for complete type reference
* **Designing structures** - See `atomspace://docs/designing-structures`
* **Creating atoms** - See `atomspace://docs/create-atom`

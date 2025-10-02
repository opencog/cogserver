# Querying the AtomSpace

This prompt helps you query and explore Atoms in the AtomSpace effectively.

## Overview

The AtomSpace can contain millions of Atoms. Before querying, always check counts to avoid overwhelming results.

## Step-by-Step Query Process

### 1. Check What's in the AtomSpace

**Tool**: `reportCounts`

**Purpose**: Get a summary of all Atom types and their counts

**Example Response**:
```json
{
  "ConceptNode": 150,
  "PredicateNode": 25,
  "ListLink": 80,
  "EvaluationLink": 45
}
```

**Best Practice**: Always run this first to understand the AtomSpace contents.

### 2. Check if Specific Atoms Exist

#### For Nodes

**Tool**: `haveNode`

**Parameters**:
- `type`: The Node type (e.g., "ConceptNode")
- `name`: The Node name

**Example**:
```json
{
  "type": "ConceptNode",
  "name": "cat"
}
```

**Returns**: `true` or `false`

#### For Links

**Tool**: `haveLink`

**Parameters**:
- `type`: The Link type
- `outgoing`: Array of Atoms in the link

**Example**:
```json
{
  "type": "ListLink",
  "outgoing": [
    {"type": "ConceptNode", "name": "cat"},
    {"type": "ConceptNode", "name": "dog"}
  ]
}
```

#### For Any Atom

**Tool**: `haveAtom`

**Example**:
```json
{
  "type": "ConceptNode",
  "name": "bird"
}
```

### 3. Get All Atoms of a Specific Type

**Tool**: `getAtoms`

**Parameters**:
- `type`: Atom type to retrieve
- `subclass` (optional): Include subtypes (default: false)

**Important**: Always check `reportCounts` first! If there are hundreds of Atoms, warn the user.

**Example: Get all ConceptNodes**
```json
{
  "type": "ConceptNode"
}
```

**Example: Get all Links (including subtypes)**
```json
{
  "type": "Link",
  "subclass": true
}
```

### 4. Find Links Containing an Atom

**Tool**: `getIncoming`

**Purpose**: Find all Links that reference a specific Atom

**Example: Find all Links containing ConceptNode "cat"**
```json
{
  "type": "ConceptNode",
  "name": "cat"
}
```

**Use Cases**:
- Find relationships involving a concept
- Discover what predicates apply to an entity
- Navigate the hypergraph structure

### 5. Explore Type Hierarchies

#### Get Subtypes

**Tool**: `getSubTypes`

**Parameters**:
- `type`: Type to get children of
- `recursive` (optional): Get all descendants (default: false)

**Example: Get all Node subtypes**
```json
{
  "type": "Node",
  "recursive": true
}
```

#### Get Supertypes

**Tool**: `getSuperTypes`

**Parameters**:
- `type`: Type to get parents of
- `recursive` (optional): Get all ancestors (default: false)

**Example: Get parent types of ConceptNode**
```json
{
  "type": "ConceptNode"
}
```

## Query Patterns

### Pattern 1: Explore a Concept

**Goal**: Find everything related to "cat"

**Steps**:
1. Check if the concept exists: `haveNode("ConceptNode", "cat")`
2. Find all Links containing it: `getIncoming(ConceptNode "cat")`
3. Examine the returned Links to understand relationships

### Pattern 2: Find All of a Relationship Type

**Goal**: Find all "has-color" relationships

**Steps**:
1. Get count: `reportCounts()` → check EvaluationLink count
2. Get all EvaluationLinks: `getAtoms("EvaluationLink")`
3. Filter for ones containing PredicateNode "has-color"

### Pattern 3: Discover Available Types

**Goal**: See what Node types are being used

**Steps**:
1. Get all Node subtypes: `getSubTypes("Node", recursive=true)`
2. Check counts: `reportCounts()` to see which are actually used
3. Get examples: `getAtoms(type)` for interesting types (if count < 100)

## Best Practices

### Before Querying Large Sets

```
1. Run reportCounts()
2. If type has > 200 Atoms:
   - Warn user about the count
   - Ask for confirmation
   - Suggest filtering strategies
```

### When User Says "Show me..."

```
1. Determine the Atom type involved
2. Check count with reportCounts()
3. If reasonable, use getAtoms()
4. If too many, ask user to be more specific
```

### When User Says "Find all X related to Y"

```
1. Verify Y exists with haveNode/haveLink
2. Use getIncoming(Y) to find Links containing Y
3. Filter the results for type X
4. Present the filtered results
```

## Common Mistakes to Avoid

❌ **Don't**: Call `getAtoms("Node")` without checking counts first
✅ **Do**: Run `reportCounts()`, then `getAtoms("ConceptNode")` for specific type

❌ **Don't**: Return thousands of Atoms in one query
✅ **Do**: Warn user and ask for confirmation or more specific criteria

❌ **Don't**: Assume AtomSpace contents are static
✅ **Do**: Re-query when asked to repeat a task (contents may have changed)

❌ **Don't**: Query for just "Atom" type
✅ **Do**: Query for specific types like "ConceptNode" or "ListLink"

## Example Query Session

**User**: "What concepts are in the AtomSpace?"

**Steps**:
1. `reportCounts()` → See that there are 50 ConceptNodes
2. `getAtoms({"type": "ConceptNode"})` → Get all 50
3. Present the list to user

**User**: "What relationships involve 'cat'?"

**Steps**:
1. `haveNode({"type": "ConceptNode", "name": "cat"})` → Verify it exists
2. `getIncoming({"type": "ConceptNode", "name": "cat"})` → Get all Links
3. Analyze and present the relationships found

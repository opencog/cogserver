# Creating Atoms in the AtomSpace

This prompt helps you create Atoms (Nodes and Links) in the AtomSpace using Atomese s-expressions.

## Key Concepts

1. **Nodes** - Have a type and a name. Names are UTF-8 strings.
2. **Links** - Have a type and an outgoing set (list of Atoms).
3. **Atoms are globally unique** - Creating the same Atom twice returns the same Atom.
4. **Atomese** - The s-expression language: `(Concept "name")` for Nodes, `(List ...)` for Links.

## Atomese Format

All MCP commands use **s-expressions** wrapped in an `atomese` field:

**Input**: `{"atomese": "(Concept \"cat\")"}`

**Output**: `(ConceptNode "cat")`

**Note**: In input, you can omit "Node" and "Link" suffixes (write `Concept` instead of `ConceptNode`). In output, the full type names are always used.

## Common Atom Types

### Node Types
- `Concept` - Represents a concept (e.g., "cat", "dog", "color")
- `Predicate` - Represents a predicate/property (e.g., "is-red", "has-color")
- `Number` - Represents a number (stored as string)
- `Variable` - Represents a variable in patterns

### Link Types
- `List` - Ordered list of Atoms
- `Set` - Unordered set of Atoms
- `Evaluation` - Relates a predicate to arguments
- `Inheritance` - Represents "is-a" relationships
- `Member` - Represents set membership

## Creating Nodes

**Tool**: `makeAtom`

**Example: Create a Concept**
```json
{
  "atomese": "(Concept \"cat\")"
}
```

**Example: Create a Predicate**
```json
{
  "atomese": "(Predicate \"has-color\")"
}
```

## Creating Links

**Tool**: `makeAtom`

**Example: Create a List**
```json
{
  "atomese": "(List (Concept \"cat\") (Concept \"mat\"))"
}
```

**Example: Create an Evaluation**
```json
{
  "atomese": "(Evaluation (Predicate \"has-color\") (List (Concept \"sky\") (Concept \"blue\")))"
}
```

## Best Practices

1. **Check if Atom exists first** - Use `haveNode` or `haveLink` before creating
2. **Build from bottom up** - Create leaf Nodes before Links that contain them
3. **Use descriptive names** - Node names should be clear and meaningful
4. **Check Atom counts** - Use `reportCounts` to monitor AtomSpace size

## Common Patterns

### Creating a Property Assertion
"The sky is blue"
```json
{
  "atomese": "(Evaluation (Predicate \"has-color\") (List (Concept \"sky\") (Concept \"blue\")))"
}
```

### Creating an Inheritance Relationship
"A cat is an animal"
```json
{
  "atomese": "(Inheritance (Concept \"cat\") (Concept \"animal\"))"
}
```

### Creating a Graph Edge
For representing directed graphs:
```json
{
  "atomese": "(Edge (Predicate \"follows\") (List (Item \"Alice\") (Item \"Bob\")))"
}
```

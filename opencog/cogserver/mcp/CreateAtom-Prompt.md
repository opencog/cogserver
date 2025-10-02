# Creating Atoms in the AtomSpace

This prompt helps you create Atoms (Nodes and Links) in the AtomSpace correctly.

## Key Concepts

1. **Nodes** - Have a type and a name. Names are UTF-8 strings.
2. **Links** - Have a type and an outgoing set (list of Atoms).
3. **Atoms are globally unique** - Creating the same Atom twice returns the same Atom.

## Common Atom Types

### Node Types
- `ConceptNode` - Represents a concept (e.g., "cat", "dog", "color")
- `PredicateNode` - Represents a predicate/property (e.g., "is-red", "has-color")
- `NumberNode` - Represents a number (stored as string)
- `VariableNode` - Represents a variable in patterns

### Link Types
- `ListLink` - Ordered list of Atoms
- `SetLink` - Unordered set of Atoms
- `EvaluationLink` - Relates a predicate to arguments
- `InheritanceLink` - Represents "is-a" relationships
- `MemberLink` - Represents set membership

## Creating Nodes

**Tool**: `makeAtom`

**Example: Create a ConceptNode**
```json
{
  "type": "ConceptNode",
  "name": "cat"
}
```

**Example: Create a PredicateNode**
```json
{
  "type": "PredicateNode",
  "name": "has-color"
}
```

## Creating Links

**Tool**: `makeAtom`

**Example: Create a ListLink**
```json
{
  "type": "ListLink",
  "outgoing": [
    {"type": "ConceptNode", "name": "cat"},
    {"type": "ConceptNode", "name": "mat"}
  ]
}
```

**Example: Create an EvaluationLink**
```json
{
  "type": "EvaluationLink",
  "outgoing": [
    {"type": "PredicateNode", "name": "has-color"},
    {
      "type": "ListLink",
      "outgoing": [
        {"type": "ConceptNode", "name": "sky"},
        {"type": "ConceptNode", "name": "blue"}
      ]
    }
  ]
}
```

## Creating Multiple Atoms

**Tool**: `loadAtoms`

**Example: Create several Atoms at once**
```json
{
  "atoms": [
    {"type": "ConceptNode", "name": "cat"},
    {"type": "ConceptNode", "name": "dog"},
    {"type": "ConceptNode", "name": "bird"}
  ]
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
  "type": "EvaluationLink",
  "outgoing": [
    {"type": "PredicateNode", "name": "has-color"},
    {
      "type": "ListLink",
      "outgoing": [
        {"type": "ConceptNode", "name": "sky"},
        {"type": "ConceptNode", "name": "blue"}
      ]
    }
  ]
}
```

### Creating an Inheritance Relationship
"A cat is an animal"
```json
{
  "type": "InheritanceLink",
  "outgoing": [
    {"type": "ConceptNode", "name": "cat"},
    {"type": "ConceptNode", "name": "animal"}
  ]
}
```

### Creating a Graph Edge
For representing directed graphs:
```json
{
  "type": "EdgeLink",
  "outgoing": [
    {"type": "PredicateNode", "name": "follows"},
    {
      "type": "ListLink",
      "outgoing": [
        {"type": "ItemNode", "name": "Alice"},
        {"type": "ItemNode", "name": "Bob"}
      ]
    }
  ]
}
```

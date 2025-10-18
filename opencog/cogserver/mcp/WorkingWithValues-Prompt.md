# Working with Values

This prompt explains how to work with the Value system in the AtomSpace using Atomese s-expressions.

## Value System Overview

- Every Atom can have **key-value pairs** attached to it
- Keys are Atoms (usually Predicates)
- Values can be:
  - FloatValue (vectors of floats)
  - StringValue (vectors of strings)
  - LinkValue (vectors of other Values)
  - Other specialized Value types

## Atomese Format

All MCP commands use **s-expressions** for Atoms:
- Input: `{"atomese": "(Concept \"cat\")"}` (shortened forms accepted)
- Output: `(ConceptNode "cat")` (full type names always used)

## Key-Value Pairs

### Getting All Keys on an Atom

**Tool**: `getKeys`

**Example**:
```json
{
  "atomese": "(Concept \"cat\")"
}
```

**Returns**: `(list (PredicateNode "weight")(PredicateNode "age")...)`

### Getting All Values on an Atom

**Tool**: `getValues`

**Example**:
```json
{
  "atomese": "(Concept \"cat\")"
}
```

**Returns**: S-expression with all key-value pairs

### Getting a Specific Value

**Tool**: `getValueAtKey`

**Example: Get the value at key "weight"**
```json
{
  "atomese": "(Concept \"cat\")",
  "key": {
    "atomese": "(Predicate \"weight\")"
  }
}
```

### Setting a Value

**Tool**: `setValue`

**Example: Store a numeric measurement**
```json
{
  "atomese": "(Concept \"cat\")",
  "key": {
    "atomese": "(Predicate \"weight\")"
  },
  "value": {
    "type": "FloatValue",
    "value": [4.5]
  }
}
```

**Example: Store multiple measurements**
```json
{
  "atomese": "(Concept \"experiment-001\")",
  "key": {
    "atomese": "(Predicate \"measurements\")"
  },
  "value": {
    "type": "FloatValue",
    "value": [1.2, 3.4, 5.6, 7.8, 9.0]
  }
}
```

**Example: Store text data**
```json
{
  "atomese": "(Concept \"document-123\")",
  "key": {
    "atomese": "(Predicate \"tags\")"
  },
  "value": {
    "type": "StringValue",
    "value": ["science", "research", "ai"]
  }
}
```

## Common Value Types

### FloatValue
Stores vectors of floating-point numbers
```json
{
  "type": "FloatValue",
  "value": [1.0, 2.5, 3.7]
}
```

**Use cases**: Embeddings, measurements, probabilities, coordinates

### StringValue
Stores vectors of strings
```json
{
  "type": "StringValue",
  "value": ["tag1", "tag2", "tag3"]
}
```

**Use cases**: Labels, categories, annotations, metadata

## Practical Examples

### Example 1: Storing Entity Properties

**Goal**: Store age and weight for a person

```json
// Create the person
{"atomese": "(Concept \"Alice\")"}

// Store age
{
  "atomese": "(Concept \"Alice\")",
  "key": {"atomese": "(Predicate \"age\")"},
  "value": {"type": "FloatValue", "value": [25.0]}
}

// Store weight
{
  "atomese": "(Concept \"Alice\")",
  "key": {"atomese": "(Predicate \"weight-kg\")"},
  "value": {"type": "FloatValue", "value": [65.5]}
}
```

### Example 2: Storing Embeddings

**Goal**: Store a 3D vector embedding

```json
{
  "atomese": "(Concept \"word-cat\")",
  "key": {"atomese": "(Predicate \"embedding\")"},
  "value": {
    "type": "FloatValue",
    "value": [0.123, -0.456, 0.789]
  }
}
```

### Example 3: Tagging with Metadata

**Goal**: Add multiple tags to a document

```json
{
  "atomese": "(Concept \"paper-2024-001\")",
  "key": {"atomese": "(Predicate \"keywords\")"},
  "value": {
    "type": "StringValue",
    "value": ["machine learning", "neural networks", "transformers"]
  }
}
```

## Best Practices

1. **Use Predicates as keys**: This is the convention
2. **Choose appropriate Value types**: FloatValue for numbers, StringValue for text
4. **Vector nature**: Remember that Values are vectors, not single items
5. **Immutability**: Atoms themselves are immutable, but Values can be changed

## Common Patterns

### Store and Retrieve Numeric Data
```json
// Store
{
  "atomese": "(Concept \"data\")",
  "key": {"atomese": "(Predicate \"values\")"},
  "value": {"type": "FloatValue", "value": [1.0, 2.0]}
}

// Retrieve
{
  "atomese": "(Concept \"data\")",
  "key": {"atomese": "(Predicate \"values\")"}
}
```

### Store and Retrieve Metadata
```json
// Store
{
  "atomese": "(Concept \"item\")",
  "key": {"atomese": "(Predicate \"metadata\")"},
  "value": {"type": "StringValue", "value": ["tag1", "tag2"]}
}

// Retrieve all keys
{"atomese": "(Concept \"item\")"}

// Retrieve all values
{"atomese": "(Concept \"item\")"}
```

# Working with Values

This prompt explains how to work with the Value system in the AtomSpace.

## Value System Overview

- Every Atom can have **key-value pairs** attached to it
- Keys are Atoms (usually PredicateNodes)
- Values can be:
  - FloatValue (vectors of floats)
  - StringValue (vectors of strings)
  - LinkValue (vectors of other Values)
  - TruthValue (strength and confidence)
  - Other specialized Value types

## Key-Value Pairs

### Getting All Keys on an Atom

**Tool**: `getKey`

**Example**:
```json
{
  "type": "ConceptNode",
  "name": "cat"
}
```

**Returns**: Array of Atoms that are used as keys

### Getting All Values on an Atom

**Tool**: `getValues`

**Example**:
```json
{
  "type": "ConceptNode",
  "name": "cat"
}
```

**Returns**: Object with all key-value pairs

### Getting a Specific Value

**Tool**: `getValueAtKey`

**Example: Get the value at key "weight"**
```json
{
  "type": "ConceptNode",
  "name": "cat",
  "key": {
    "type": "PredicateNode",
    "name": "weight"
  }
}
```

### Setting a Value

**Tool**: `setValue`

**Example: Store a numeric measurement**
```json
{
  "type": "ConceptNode",
  "name": "cat",
  "key": {
    "type": "PredicateNode",
    "name": "weight"
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
  "type": "ConceptNode",
  "name": "experiment-001",
  "key": {
    "type": "PredicateNode",
    "name": "measurements"
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
  "type": "ConceptNode",
  "name": "document-123",
  "key": {
    "type": "PredicateNode",
    "name": "tags"
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

### SimpleTruthValue
Stores probabilistic truth information
```json
{
  "type": "SimpleTruthValue",
  "value": [0.8, 0.9]
}
```

**Use cases**: Uncertain facts, learned knowledge, probabilistic reasoning

## Practical Examples

### Example 1: Storing Entity Properties

**Goal**: Store age and weight for a person

```json
// Create the person
{"type": "ConceptNode", "name": "Alice"}

// Store age
{
  "type": "ConceptNode",
  "name": "Alice",
  "key": {"type": "PredicateNode", "name": "age"},
  "value": {"type": "FloatValue", "value": [25.0]}
}

// Store weight
{
  "type": "ConceptNode",
  "name": "Alice",
  "key": {"type": "PredicateNode", "name": "weight-kg"},
  "value": {"type": "FloatValue", "value": [65.5]}
}
```

### Example 2: Storing Embeddings

**Goal**: Store a 3D vector embedding

```json
{
  "type": "ConceptNode",
  "name": "word-cat",
  "key": {"type": "PredicateNode", "name": "embedding"},
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
  "type": "ConceptNode",
  "name": "paper-2024-001",
  "key": {"type": "PredicateNode", "name": "keywords"},
  "value": {
    "type": "StringValue",
    "value": ["machine learning", "neural networks", "transformers"]
  }
}
```

### Example 4: Storing Truth Values

**Goal**: Store probabilistic/uncertain knowledge using the truth-value key

```json
// Create the relationship
{
  "type": "InheritanceLink",
  "outgoing": [
    {"type": "ConceptNode", "name": "Tweety"},
    {"type": "ConceptNode", "name": "bird"}
  ]
}

// Set its truth value using setValue with the predefined truth-value key
{
  "type": "InheritanceLink",
  "outgoing": [
    {"type": "ConceptNode", "name": "Tweety"},
    {"type": "ConceptNode", "name": "bird"}
  ],
  "key": {
    "type": "PredicateNode",
    "name": "*-TruthValueKey-*"
  },
  "value": {
    "type": "SimpleTruthValue",
    "value": [0.95, 0.90]
  }
}
```

## Best Practices

1. **Use PredicateNodes as keys**: This is the convention
2. **Choose appropriate Value types**: FloatValue for numbers, StringValue for text
3. **Truth Values**: Use the predefined `*-TruthValueKey-*` key with setValue/getValueAtKey
4. **Vector nature**: Remember that Values are vectors, not single items
5. **Immutability**: Atoms themselves are immutable, but Values can be changed

## Common Patterns

### Store and Retrieve Numeric Data
```javascript
// Store
setValue(atom, "key-name", FloatValue([1.0, 2.0]))

// Retrieve
getValueAtKey(atom, "key-name")
```

### Store and Retrieve Metadata
```javascript
// Store
setValue(atom, "metadata", StringValue(["tag1", "tag2"]))

// Retrieve all keys
getKey(atom)

// Retrieve all values
getValues(atom)
```

### Work with Uncertain Knowledge
```javascript
// Create relationship
makeAtom(InheritanceLink(concept1, concept2))

// Add uncertainty using setValue with truth-value key
setValue(InheritanceLink(concept1, concept2), "*-TruthValueKey-*", SimpleTruthValue([strength, confidence]))

// Check uncertainty using getValueAtKey
getValueAtKey(InheritanceLink(concept1, concept2), "*-TruthValueKey-*")
```

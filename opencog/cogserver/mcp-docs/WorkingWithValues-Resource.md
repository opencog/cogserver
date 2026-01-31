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
- Values are attached to Atoms but are NOT stored in the AtomSpace themselves

## Atomese Format

### Atoms
All MCP commands use **s-expressions** for Atoms:
- Input: `{"atomese": "(Concept \"cat\")"}` (shortened forms accepted)
- Output: `(ConceptNode "cat")` (full type names always used)

### Values
Values can also be specified using s-expressions:
- `{"atomese": "(FloatValue 1.5 2.7 3.14)"}`
- `{"atomese": "(StringValue \"hello\" \"world\")"}`
- `{"atomese": "(VoidValue)"}`

**Critical Rule**: Values are NOT Atoms. Links can ONLY contain Atoms in their
outgoing set - never Values. This means you cannot write Atomese like:
```
(SetValue (Concept "x") (Predicate "key") (FloatValue 1.0))  ; ILLEGAL!
```
The above is invalid because SetValue is a Link, and FloatValue is a Value,
not an Atom. Values cannot appear inside Links.

This is why the MCP `setValue` tool exists: it allows you to specify the Value
as a separate argument, outside of any Link structure. The Value is passed to
the tool, not embedded in Atomese.

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
  "key": {"atomese": "(Predicate \"weight\")"},
  "value": {"atomese": "(FloatValue 4.5)"}
}
```

**Example: Store multiple measurements**
```json
{
  "atomese": "(Concept \"experiment-001\")",
  "key": {"atomese": "(Predicate \"measurements\")"},
  "value": {"atomese": "(FloatValue 1.2 3.4 5.6 7.8 9.0)"}
}
```

**Example: Store text data**
```json
{
  "atomese": "(Concept \"document-123\")",
  "key": {"atomese": "(Predicate \"tags\")"},
  "value": {"atomese": "(StringValue \"science\" \"research\" \"ai\")"}
}
```

## Common Value Types

### FloatValue
Stores vectors of floating-point numbers: `(FloatValue 1.0 2.5 3.7)`

**Use cases**: Embeddings, measurements, probabilities, coordinates

### StringValue
Stores vectors of strings: `(StringValue "tag1" "tag2" "tag3")`

**Use cases**: Labels, categories, annotations, metadata

### VoidValue
A Value with no content: `(VoidValue)`

**Use cases**: Sending messages to ObjectNodes that require no arguments.
For example, opening or closing a StorageNode:
```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-open-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

## Practical Examples

### Example 1: Storing Entity Properties

**Goal**: Store age and weight for a person

```json
// Create the person (using makeAtom tool)
{"atomese": "(Concept \"Alice\")"}

// Store age (using setValue tool)
{
  "atomese": "(Concept \"Alice\")",
  "key": {"atomese": "(Predicate \"age\")"},
  "value": {"atomese": "(FloatValue 25.0)"}
}

// Store weight (using setValue tool)
{
  "atomese": "(Concept \"Alice\")",
  "key": {"atomese": "(Predicate \"weight-kg\")"},
  "value": {"atomese": "(FloatValue 65.5)"}
}
```

### Example 2: Storing Embeddings

**Goal**: Store a 3D vector embedding

```json
{
  "atomese": "(Concept \"word-cat\")",
  "key": {"atomese": "(Predicate \"embedding\")"},
  "value": {"atomese": "(FloatValue 0.123 -0.456 0.789)"}
}
```

### Example 3: Tagging with Metadata

**Goal**: Add multiple tags to a document

```json
{
  "atomese": "(Concept \"paper-2024-001\")",
  "key": {"atomese": "(Predicate \"keywords\")"},
  "value": {"atomese": "(StringValue \"machine learning\" \"neural networks\" \"transformers\")"}
}
```

## Pattern Matching on Values with FilterLink

**Important**: Since Values don't have Incoming Sets, you **cannot** use MeetLink or QueryLink on them.

**Use FilterLink instead** for pattern matching on Value structures.

### FilterLink Overview

FilterLink is a pattern matcher (not a query/traversal engine) designed for Values:
- Works on LinkValue, Section, and other Value structures
- Uses LinkSignature to specify Value types in patterns
- Uses GlobNode to match variable-length sequences
- Returns matched Values (not traversing a graph)

### Basic FilterLink Structure

```scheme
(Filter
  (Rule
    <variable-declarations>
    <pattern-to-match>
    <rewrite-template>)
  <value-structure-to-filter>)
```

### Example: Extracting from LinkValue

**Input LinkValue structure:**
```scheme
(LinkValue
  (LinkValue
    (Word "cat")
    (Word "sat")
    (Word "mat")))
```

**FilterLink to extract words:**
```scheme
(Filter
  (Rule
    (VariableList
      (TypedVariable (Variable "$lv") (Type 'LinkValue))
      (TypedVariable (Variable "$word") (Type 'Word)))
    (LinkSignature (Type 'LinkValue)
      (Variable "$lv")
      (Variable "$word"))
    (Variable "$word"))
  <input-linkvalue>)
```

**Key differences from MeetLink/QueryLink:**
- LinkSignature declares the Value type being matched
- No graph traversal - pure pattern matching on Value structure
- GlobNode works perfectly in FilterLink patterns
- Cannot use Incoming Sets (Values don't have them)

### When to Use FilterLink

**Use FilterLink when:**
- Working with LinkValue, Section, or other Values
- Processing results from LgParse, LgParseBonds, etc. (these return Values)
- Pattern matching on hierarchical Value structures
- Extracting specific elements from Value sequences

**Do NOT use MeetLink/QueryLink for:**
- LinkValue structures
- Values returned by executable Atoms
- Any Value type (FloatValue, StringValue, etc.)

## Best Practices

1. **Use Predicates as keys**: This is the convention
2. **Choose appropriate Value types**: FloatValue for numbers, StringValue for text
3. **Pattern matching**: Use FilterLink for Values, MeetLink/QueryLink for Atoms
4. **Vector nature**: Remember that Values are vectors, not single items
5. **Immutability**: Atoms themselves are immutable, but Values can be changed
6. **No queries on Values**: Cannot use query engine on Values - use FilterLink instead

## Common Patterns

### Store and Retrieve Numeric Data
```json
// Store (using setValue tool)
{
  "atomese": "(Concept \"data\")",
  "key": {"atomese": "(Predicate \"values\")"},
  "value": {"atomese": "(FloatValue 1.0 2.0)"}
}

// Retrieve (using getValueAtKey tool)
{
  "atomese": "(Concept \"data\")",
  "key": {"atomese": "(Predicate \"values\")"}
}
```

### Store and Retrieve Metadata
```json
// Store (using setValue tool)
{
  "atomese": "(Concept \"item\")",
  "key": {"atomese": "(Predicate \"metadata\")"},
  "value": {"atomese": "(StringValue \"tag1\" \"tag2\")"}
}

// Retrieve all keys (using getKeys tool)
{"atomese": "(Concept \"item\")"}

// Retrieve all values (using getValues tool)
{"atomese": "(Concept \"item\")"}
```

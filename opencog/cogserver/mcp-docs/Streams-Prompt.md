# Streams in Atomese

This prompt explains how to work with Streams in the AtomSpace - a powerful system for creating data processing pipelines that handle dynamically updating and flowing data.

## Overview

### What Are Streams?

**Streams** are a type of Value that represent dynamic, flowing data in Atomese. Unlike static Values (like FloatValue or StringValue), streams update or advance each time they are accessed, making them ideal for:

- Real-time data processing pipelines
- Dynamically computed values (like futures/promises)
- Sequential processing of collections
- Data transformations and filtering

### CRITICAL: Streams Are Values, Not Atoms

This distinction is fundamental to understanding how to work with streams:

**Streams are Values:**
- **Cannot be stored** directly in the AtomSpace
- **Do NOT have an Incoming Set** - no back-links, no indexing
- **Cannot be traversed** by MeetLink/QueryLink/BindLink
- **Use FilterLink** for pattern matching on streams

**How to work with Streams:**
- Attach streams to Atoms using SetValue (Atoms act as "anchors")
- Access streams using ValueOf or StreamValueOf
- Pattern match on streams using FilterLink (NOT MeetLink)
- Process streams using DrainLink to exhaust them

### Key Stream Characteristics

1. **Dynamic updating**: Each access returns fresh data
2. **Sequential advancement**: Streams move forward with each reference
3. **End-of-stream signal**: VoidValue or zero-length LinkValue marks completion
4. **No caching**: Values are computed/retrieved on-demand

## Core Stream Types

### FormulaStream

**Purpose**: Dynamically computes numeric values from formulas

**Behavior**:
- Always returns a FloatValue
- Recomputes formula every time it's accessed
- Perfect for live calculations and metrics

**Basic Example**:
```scheme
; Create a formula stream that computes current time
(define time-stream (FormulaStream (TimeLink)))

; Each access returns the current timestamp
(cog-value->list time-stream)  ; Returns current time
(sleep 1)
(cog-value->list time-stream)  ; Returns new current time
```

**Formula with Arithmetic**:
```scheme
; Define helper functions for TruthValue components
(define tvkey (Predicate "*-TruthValueKey-*"))
(define (strength-of ATOM)
  (ElementOf (Number 0) (ValueOf ATOM tvkey)))
(define (confidence-of ATOM)
  (ElementOf (Number 1) (ValueOf ATOM tvkey)))

; Create a dynamic formula stream
(define tv-stream
  (FormulaStream
    (Minus
      (Number 1)
      (Times
        (strength-of (Concept "A"))
        (strength-of (Concept "B"))))
    (Times
      (confidence-of (Concept "A"))
      (confidence-of (Concept "B")))))

; When A or B's values change, the stream automatically reflects it
(cog-set-value! (Concept "A") tvkey (FloatValue 0.9 0.2))
(cog-set-value! (Concept "B") tvkey (FloatValue 0.4 0.7))
(cog-value->list tv-stream)  ; Returns dynamically computed result
```

**Use Cases**:
- Real-time metric computation
- Dynamic relationship strengths
- Derived values that update automatically
- Statistical calculations on changing data

### FutureStream

**Purpose**: Wraps executable Atoms to create futures/promises

**Behavior**:
- Executes wrapped Atoms every time it's accessed
- Returns arbitrary Value types (not just FloatValue)
- Implements the futures/promises pattern

**Basic Example**:
```scheme
; Wrap TimeLink to get current time on each access
(define fut (FutureStream (TimeLink)))

; Each access executes TimeLink and returns result
(cog-value-ref fut 0)  ; Current timestamp
(sleep 1)
(cog-value-ref fut 0)  ; New timestamp
```

**Multiple Sources**:
```scheme
; Combine multiple executable atoms
(define multi-stream
  (FutureStream
    (TimeLink)
    (ValueOf (Concept "foo") (Predicate "key"))))

; Returns vector with time and foo's value
(cog-value->list multi-stream)
```

**Use Cases**:
- Lazy evaluation
- Computed values that update on demand
- Combining multiple dynamic data sources
- Deferring expensive computations

### FlatStream

**Purpose**: Flattens lists into sequential streams of individual items

**Behavior**:
- Takes a list/collection and yields items one at a time
- Advances to next item with each access
- Can loop forever on static lists
- Blocks when stream temporarily empty (for live streams)

**Basic Example**:
```scheme
; Create a list of items
(define item-list
  (OrderedLink
    (Item "a")
    (Item "b")
    (Item "c")
    (Edge (Predicate "relation") (List (Item "d") (Item "e")))
    (Item "f")))

; Wrap with FlatStream
(define fs (FlatStream item-list))

; Each access returns next item
(cog-value->list fs)  ; Returns (Item "a")
(cog-value->list fs)  ; Returns (Item "b")
(cog-value->list fs)  ; Returns (Item "c")
(cog-value->list fs)  ; Returns (Edge ...)
```

**Attaching to AtomSpace**:
```scheme
; Attach stream to a well-known location
(cog-execute!
  (SetValue (Concept "foo") (Predicate "bar")
    (CollectionOf (Type 'FlatStream) (OrderedLink item-list))))

; Access via ValueOf
(define stream-ref (ValueOf (Concept "foo") (Predicate "bar")))
(cog-execute! stream-ref)  ; Returns next item
(cog-execute! stream-ref)  ; Returns next item
```

**Finite Streams**:
```scheme
; Terminate with VoidValue to mark end
(define finite-list
  (LinkValue
    (Concept "A")
    (Concept "B")
    (Concept "C")
    (VoidValue)))  ; End-of-stream marker

(define finite-stream (FlatStream finite-list))
; Stream will exhaust after 3 items
```

**Use Cases**:
- Processing lists item-by-item
- Streaming data from collections
- Pipeline input from static data
- Converting batches to streams

### SortedStream

**Purpose**: Maintains items in sorted order while streaming

**Behavior**:
- Pulls items from upstream, sorts them, offers to downstream
- Thread-safe buffer for concurrent read/write
- Uses custom sort relations defined in Atomese
- Not a stable sort (order of equal elements undefined)

**Basic Example**:
```scheme
; Define a sort relation (descending by size)
(define greater-or-equal-relation
  (Lambda
    (VariableList (Variable "$left") (Variable "$right"))
    (Or
      (GreaterThan
        (SizeOf (Variable "$left"))
        (SizeOf (Variable "$right")))
      (Equal
        (SizeOf (Variable "$left"))
        (SizeOf (Variable "$right"))))))

; Create list of items
(define item-list
  (OrderedLink
    (Item "a")                    ; size 1
    (Item "b")                    ; size 1
    (Link (Item "p") (Item "q"))  ; size 2
    (Item "c")))                  ; size 1

; Create sorted stream
(define sorted (SortedStream greater-or-equal-relation item-list))

; Access in sorted order (largest first)
(cog-value->list sorted)  ; Returns (Link ...) - size 2
(cog-value->list sorted)  ; Returns (Item "a") or "b" or "c" - size 1
```

**Ascending Order**:
```scheme
; Sort smallest first
(define less-or-equal-relation
  (Lambda
    (VariableList (Variable "$left") (Variable "$right"))
    (Not
      (GreaterThan
        (SizeOf (Variable "$left"))
        (SizeOf (Variable "$right"))))))

(define ascending (SortedStream less-or-equal-relation item-list))
```

**Deduplication Effect**:
```scheme
; Using strict greater-than (not greater-or-equal) causes deduplication
(define strict-greater
  (Lambda
    (VariableList (Variable "$left") (Variable "$right"))
    (GreaterThan
      (SizeOf (Variable "$left"))
      (SizeOf (Variable "$right")))))

; Only one item of each size will appear
(define dedup-stream (SortedStream strict-greater item-list))
```

**Use Cases**:
- Priority queues
- Ordered data processing
- Deduplication by custom criteria
- Sorting live streaming data

## Creating and Attaching Streams

### Creating Streams Directly

```scheme
; Direct creation in Scheme
(define my-stream (FormulaStream (Plus (Number 1) (Number 2))))
(define my-future (FutureStream (TimeLink)))
(define my-flat (FlatStream (OrderedLink (Item "a") (Item "b"))))
```

### Using CollectionOfLink

**Purpose**: Wrap executables into stream types using pure Atomese

**Syntax**:
```scheme
(CollectionOf (Type 'StreamType) <executable-expression>)
```

**Example - Creating FormulaStream**:
```scheme
; Define a formula as ExecutionOutput
(DefineLink
  (DefinedProcedure "compute-value")
  (Lambda
    (VariableList (Variable "$X") (Variable "$Y"))
    (Plus
      (FloatValueOf (Variable "$X") tvkey)
      (FloatValueOf (Variable "$Y") tvkey))))

; Wrap in FormulaStream using CollectionOfLink
(cog-execute!
  (SetValue (Concept "result") (Predicate "dynamic-sum")
    (CollectionOf (Type 'FormulaStream)
      (OrderedLink
        (ExecutionOutput
          (DefinedProcedure "compute-value")
          (List (Concept "A") (Concept "B")))))))

; Access the stream
(cog-value (Concept "result") (Predicate "dynamic-sum"))
```

**Example - Creating FlatStream**:
```scheme
(cog-execute!
  (SetValue (Anchor "data-source") (Predicate "item-stream")
    (CollectionOf (Type 'FlatStream)
      (OrderedLink
        (Item "first")
        (Item "second")
        (Item "third")))))
```

### Attaching Streams to Atoms

**Why Attach Streams?**

Since streams are Values (not Atoms), they cannot exist independently in the AtomSpace. They must be attached to Atoms at specific keys, creating "well-known locations" where they can be found and accessed.

**Basic Pattern**:
```scheme
; 1. Create or reference an anchor atom
(define anchor (Anchor "my-anchor"))

; 2. Attach stream using SetValue
(cog-execute!
  (SetValue anchor (Predicate "stream-key")
    (CollectionOf (Type 'FormulaStream) <formula>)))

; 3. Access via ValueOf
(define stream-ref (ValueOf anchor (Predicate "stream-key")))
(cog-execute! stream-ref)
```

**Real Example - Dynamic MI Calculation**:
```scheme
; Define mutual information formula
(DefineLink
  (DefinedProcedure "dynamic MI")
  (Lambda
    (VariableList (Variable "$L") (Variable "$R"))
    (Log2
      (Divide
        (Times
          (FloatValueOf (List (Variable "$L") (Variable "$R")) tvp)
          (FloatValueOf (AnyNode "grand total") tvp))
        (Times
          (FloatValueOf (List (Variable "$L") (Any "right wildcard")) tvp)
          (FloatValueOf (List (Any "left wildcard") (Variable "$R")) tvp))))))

; Install on specific pair
(define (install-mi-formula THING-A THING-B)
  (define pair (List THING-A THING-B))
  (cog-execute!
    (SetValue pair (Predicate "MI Key")
      (CollectionOf (Type 'FormulaStream)
        (OrderedLink
          (ExecutionOutput
            (DefinedProcedure "dynamic MI")
            pair))))))

; Now MI is always current
(install-mi-formula (Concept "hello") (Concept "world"))
(cog-value (List (Concept "hello") (Concept "world")) (Predicate "MI Key"))
```

## Processing Streams with FilterLink

### CRITICAL: FilterLink vs MeetLink/QueryLink

**Use FilterLink for:**
- Pattern matching on Values and streams
- Processing LinkValue structures
- Filtering and transforming stream items
- Pure pattern matching (no graph traversal)

**Do NOT use MeetLink/QueryLink for:**
- Values or streams (they have no Incoming Set)
- LinkValue structures
- Stream processing

### FilterLink Structure

```scheme
(Filter
  <rule-definition>
  <stream-or-value-source>)
```

**Rule Definition**: Can be RuleLink, LambdaLink, or pattern with variables

### Basic FilterLink with RuleLink

```scheme
(Filter
  (Rule
    ; Variable declarations
    (TypedVariable (Variable "$item") (Type 'ConceptNode))

    ; Pattern to match
    (Variable "$item")

    ; Rewrite template
    (Edge (Predicate "processed") (Variable "$item")))

  ; Stream source
  <stream-reference>)
```

### Example: Processing Stream Items

```scheme
; Create a stream source
(define data-anchor (Anchor "process-anchor"))

(define test-data
  (LinkValue
    (Concept "A")
    (Concept "B")
    (Concept "C")
    (VoidValue)))  ; End marker

(cog-set-value! data-anchor (Predicate "test-data") test-data)

; Create FlatStream from data
(define stream-maker
  (LinkSignature (Type 'FlatStream)
    (ValueOf data-anchor (Predicate "test-data"))))

(cog-execute!
  (SetValue data-anchor (Predicate "data-stream") stream-maker))

; Define stream reference
(define data-stream
  (ValueOf data-anchor (Predicate "data-stream")))

; Create filter that processes each item
(define counter
  (Filter
    (Rule
      (TypedVariable (Variable "$item") (Type 'ConceptNode))

      ; Match item
      (Variable "$item")

      ; Rewrite - increment count on item
      (LinkSignature (Type 'LinkValue)
        (IncrementValueOn
          (Variable "$item")
          (Predicate "item-count")
          (Number 1))))

    data-stream))

; Execute filter - processes one item
(cog-execute! counter)
```

### Example: String to Node Conversion

```scheme
; Set up string stream
(cog-set-value! (Anchor "anch") (Predicate "words")
  (LinkValue
    (StringValue "hello")
    (StringValue "world")
    (StringValue "from")
    (StringValue "stream")))

; Filter that converts strings to tagged nodes
(define string-converter
  (Filter
    (Rule
      (Variable "$strv")
      (Variable "$strv")

      ; Convert StringValue to ConceptNode and tag
      (Edge (Predicate "word")
        (LinkSignature (Type 'Concept)
          (ValueOf (Variable "$strv")))))

    (ValueOf (Anchor "anch") (Predicate "words"))))

; Execute to convert strings to atoms
(cog-execute! string-converter)

; Now query the atoms created
(define words-query
  (Meet
    (TypedVariable (Variable "$w") (Type 'Concept))
    (Edge (Predicate "word") (Variable "$w"))))

(cog-execute! words-query)  ; Returns all word concepts
```

### LinkSignature in Patterns

**LinkSignature** specifies Value types in patterns, similar to how TypedVariable specifies Atom types.

```scheme
; Pattern matching LinkValue structures
(Filter
  (Rule
    (VariableList
      (TypedVariable (Variable "$lv") (Type 'LinkValue))
      (TypedVariable (Variable "$word") (Type 'Word)))

    ; Pattern using LinkSignature
    (LinkSignature (Type 'LinkValue)
      (Variable "$lv")
      (Variable "$word"))

    ; Extract the word
    (Variable "$word"))

  <linkvalue-source>)
```

### Practical FilterLink Patterns

**Pattern 1: Filter and Count**
```scheme
(Filter
  (Rule
    (TypedVariable (Variable "$x") (Type 'Concept))
    (Variable "$x")
    (IncrementValueOn (Variable "$x") (Predicate "count") (Number 1)))
  <stream>)
```

**Pattern 2: Type Conversion**
```scheme
(Filter
  (Rule
    (Variable "$val")
    (Variable "$val")
    (LinkSignature (Type 'Concept) (ValueOf (Variable "$val"))))
  <stream>)
```

**Pattern 3: Conditional Processing**
```scheme
(Filter
  (Rule
    (TypedVariable (Variable "$item") (Type 'Concept))

    ; Only match concepts with specific property
    (Edge (Predicate "has-property") (Variable "$item"))

    ; Process matched items
    (Edge (Predicate "processed") (Variable "$item")))
  <stream>)
```

## Type Conversion with LinkSignature

### Purpose

**LinkSignatureLink** is a type constructor that creates Values of specified types when executed. It handles conversions between:
- Node types (Concept, Predicate, Word, etc.)
- Nodes and StringValues
- NumberNodes and FloatValues
- Creating stream types

### Basic Syntax

```scheme
(LinkSignature (Type 'TargetType) <arguments>)
```

### Node to Node Conversion

```scheme
; Convert PredicateNode to ConceptNode
(cog-execute!
  (LinkSignature (Type 'Concept) (Predicate "foo")))
; Returns (ConceptNode "foo")
```

### Node to StringValue

```scheme
; Extract node name as StringValue
(cog-execute!
  (SetValue (Anchor "result") (Predicate "string-key")
    (LinkSignature (Type 'StringValue) (Concept "hello"))))

(cog-value (Anchor "result") (Predicate "string-key"))
; Returns (StringValue "hello")
```

### StringValue to Node

```scheme
; Set up StringValue
(cog-set-value! (Anchor "src") (Predicate "key")
  (StringValue "world"))

; Convert to ConceptNode
(cog-execute!
  (LinkSignature (Type 'Concept)
    (ValueOf (Anchor "src") (Predicate "key"))))
; Returns (ConceptNode "world")
```

### Creating Stream Types

```scheme
; Create FlatStream using LinkSignature
(define stream-creator
  (LinkSignature (Type 'FlatStream)
    (ValueOf (Anchor "data") (Predicate "list-key"))))

; Execute to get stream instance
(define stream-instance (cog-execute! stream-creator))
```

### LinkSignature in Pipelines

```scheme
; Complete pipeline: StringValue -> ConceptNode -> Store
(cog-set-value! (Anchor "input") (Predicate "strings")
  (LinkValue
    (StringValue "alpha")
    (StringValue "beta")
    (StringValue "gamma")))

(define converter
  (Filter
    (Rule
      (Variable "$str")
      (Variable "$str")

      ; Convert and tag
      (Edge (Predicate "from-stream")
        (LinkSignature (Type 'Concept)
          (ValueOf (Variable "$str")))))

    (ValueOf (Anchor "input") (Predicate "strings"))))

(cog-execute! converter)
```

### LinkSignature vs CollectionOfLink

**LinkSignature**:
- Preserves argument structure
- Best for Value types needing multiple arguments
- Used in patterns and conversions

**CollectionOfLink**:
- Unwraps arguments before construction
- Best for wrapping executables into streams
- Used to create FormulaStream, FutureStream, etc.

```scheme
; LinkSignature - for type conversion
(LinkSignature (Type 'Concept) (Predicate "foo"))

; CollectionOfLink - for creating streams
(CollectionOf (Type 'FormulaStream)
  (OrderedLink (Plus (Number 1) (Number 2))))
```

## Exhausting Streams with DrainLink

### Purpose

**DrainLink** continuously processes a stream until it's empty, looping forever and calling execute on each item until receiving a termination signal.

### Termination Signals

**Stream ends when returning:**
- VoidValue
- Zero-length LinkValue

These represent end-of-file, closed socket, or finite stream completion.

### Basic Syntax

```scheme
(Drain <stream-or-executable>)
```

### Simple Example

```scheme
; Set up finite stream
(define data
  (LinkValue
    (Concept "A")
    (Concept "B")
    (Concept "C")
    (VoidValue)))  ; End marker

(define anchor (Anchor "drain-demo"))
(cog-set-value! anchor (Predicate "data") data)

; Create stream
(define stream-maker
  (LinkSignature (Type 'FlatStream)
    (ValueOf anchor (Predicate "data"))))

(cog-execute!
  (SetValue anchor (Predicate "stream") stream-maker))

; Reference stream
(define stream-ref (ValueOf anchor (Predicate "stream")))

; Create processor that counts items
(define processor
  (Filter
    (Rule
      (TypedVariable (Variable "$item") (Type 'Concept))
      (Variable "$item")
      (IncrementValueOn
        (Variable "$item")
        (Predicate "count")
        (Number 1)))
    stream-ref))

; Drain the stream - processes all items
(cog-execute! (Drain processor))
```

### Draining with Processing Pipeline

```scheme
; Complete example: stream -> filter -> count -> drain
(define data-anchor (Anchor "pipeline"))

(define test-data
  (LinkValue
    (Concept "A")
    (Concept "C")
    (Concept "A")
    (Concept "B")
    (Concept "A")
    (VoidValue)))

(cog-set-value! data-anchor (Predicate "source") test-data)

; Stream from source
(define stream-maker
  (LinkSignature (Type 'FlatStream)
    (ValueOf data-anchor (Predicate "source"))))

(cog-execute!
  (SetValue data-anchor (Predicate "stream") stream-maker))

(define stream-ref (ValueOf data-anchor (Predicate "stream")))

; Filter that counts each item AND increments total
(define counter
  (Filter
    (Rule
      (TypedVariable (Variable "$item") (Type 'Concept))
      (Variable "$item")

      (LinkSignature (Type 'LinkValue)
        ; Count individual item
        (IncrementValueOn
          (Variable "$item")
          (Predicate "item-count")
          (Number 1))

        ; Count total
        (IncrementValueOn
          data-anchor
          (Predicate "total-count")
          (Number 1))))

    stream-ref))

; Execute drain
(cog-execute! (Drain counter))

; Verify counts
; A appeared 3 times
(cog-value (Concept "A") (Predicate "item-count"))
; B appeared 1 time
(cog-value (Concept "B") (Predicate "item-count"))
; C appeared 1 time
(cog-value (Concept "C") (Predicate "item-count"))
; Total should be 5
(cog-value data-anchor (Predicate "total-count"))
```

### Infinite Streams with ParallelLink

For streams that never end, use ParallelLink to run drain in background thread:

```scheme
; Create infinite stream (no VoidValue terminator)
(define infinite-stream <setup-infinite-source>)

; Process in parallel
(cog-execute!
  (ParallelLink
    (Drain <processor>)))

; Main thread continues while stream processes in background
```

### DrainLink Use Cases

- Batch processing all items in a stream
- Ensuring complete pipeline execution
- Stream-to-storage operations
- Exhausting finite data sources
- Background processing with ParallelLink

## Complete Pipeline Examples

### Example 1: Dynamic Value Computation

**Goal**: Maintain automatically-updating TruthValue on ImplicationLink

```scheme
; Setup
(define tvkey (Predicate "*-TruthValueKey-*"))
(define (strength-of ATOM)
  (ElementOf (Number 0) (ValueOf ATOM tvkey)))
(define (confidence-of ATOM)
  (ElementOf (Number 1) (ValueOf ATOM tvkey)))

; Initialize values
(cog-set-value! (Concept "A") tvkey (FloatValue 0.9 0.1))
(cog-set-value! (Concept "B") tvkey (FloatValue 0.8 0.2))

; Define formula
(DefineLink
  (DefinedProcedure "compute-implication")
  (Lambda
    (VariableList (Variable "$X") (Variable "$Y"))
    (FloatColumn
      (Minus
        (Number 1)
        (Times
          (strength-of (Variable "$X"))
          (strength-of (Variable "$Y"))))
      (Times
        (confidence-of (Variable "$X"))
        (confidence-of (Variable "$Y"))))))

; Create implication with dynamic TV
(define a-implies-b (Implication (Concept "A") (Concept "B")))

(cog-execute!
  (SetValue a-implies-b tvkey
    (CollectionOf (Type 'FormulaStream)
      (OrderedLink
        (ExecutionOutput
          (DefinedProcedure "compute-implication")
          (List (Concept "A") (Concept "B")))))))

; Now TV updates automatically when A or B changes
(cog-value a-implies-b tvkey)

; Change input values
(cog-set-value! (Concept "A") tvkey (FloatValue 0.5 0.8))
(cog-set-value! (Concept "B") tvkey (FloatValue 0.3 0.7))

; TV automatically reflects new computation
(cog-value a-implies-b tvkey)
```

### Example 2: String Stream to AtomSpace

**Goal**: Convert flowing string stream into ConceptNodes stored in AtomSpace

```scheme
; Set up string stream
(cog-set-value! (Anchor "chat") (Predicate "messages")
  (LinkValue
    (StringValue "hello")
    (StringValue "how")
    (StringValue "are")
    (StringValue "you")
    (VoidValue)))

; Create conversion pipeline
(define message-processor
  (Filter
    (Rule
      (Variable "$msg")
      (Variable "$msg")

      ; Convert StringValue to ConceptNode and tag
      (Edge (Predicate "chat-word")
        (LinkSignature (Type 'Concept)
          (ValueOf (Variable "$msg")))))

    (ValueOf (Anchor "chat") (Predicate "messages"))))

; Drain the stream - converts all strings to atoms
(cog-execute! (Drain message-processor))

; Query the words that flowed through
(define chat-words-query
  (Meet
    (TypedVariable (Variable "$word") (Type 'Concept))
    (Edge (Predicate "chat-word") (Variable "$word"))))

(cog-execute! chat-words-query)
; Returns: (Set (Concept "hello") (Concept "how")
;               (Concept "are") (Concept "you"))
```

### Example 3: Sorted Priority Processing

**Goal**: Process stream items in priority order

```scheme
; Define priority relation (higher priority = larger size)
(define priority-order
  (Lambda
    (VariableList (Variable "$left") (Variable "$right"))
    (Or
      (GreaterThan
        (SizeOf (Variable "$left"))
        (SizeOf (Variable "$right")))
      (Equal
        (SizeOf (Variable "$left"))
        (SizeOf (Variable "$right"))))))

; Create items with different priorities
(define tasks
  (OrderedLink
    (Item "low")                             ; size 1
    (Link (Item "high") (Item "priority"))   ; size 2
    (Item "low2")                            ; size 1
    (Link (Item "medium"))))                 ; size 1

; Create sorted stream
(define priority-stream
  (SortedStream priority-order tasks))

; Process in priority order (high priority first)
(cog-value->list priority-stream)  ; Returns high priority item
(cog-value->list priority-stream)  ; Returns medium priority
(cog-value->list priority-stream)  ; Returns low priority
```

### Example 4: Mutual Information Pipeline

**Goal**: Maintain dynamically-updating mutual information scores

```scheme
; Setup count tracking
(define tvp (PredicateNode "*-TruthValueKey-*"))

(define (observe STRING-A STRING-B)
  (define A (Concept STRING-A))
  (define B (Concept STRING-B))
  (cog-inc-value! (List A B) tvp 1.0 2)
  (cog-inc-value! (List (AnyNode "left wildcard") B) tvp 1.0 2)
  (cog-inc-value! (List A (AnyNode "right wildcard")) tvp 1.0 2)
  (cog-inc-value! (AnyNode "grand total") tvp 1.0 2))

; Observe some pairs
(observe "hello" "world")
(observe "hello" "Sue")
(observe "goodbye" "Mike")

; Define MI formula
(DefineLink
  (DefinedProcedure "dynamic MI")
  (Lambda
    (VariableList (Variable "$L") (Variable "$R"))
    (Log2
      (Divide
        (Times
          (FloatValueOf (List (Variable "$L") (Variable "$R")) tvp)
          (FloatValueOf (AnyNode "grand total") tvp))
        (Times
          (FloatValueOf (List (Variable "$L") (Any "right wildcard")) tvp)
          (FloatValueOf (List (Any "left wildcard") (Variable "$R")) tvp))))))

; Install dynamic MI on pair
(define (install-mi THING-A THING-B)
  (define pair (List THING-A THING-B))
  (cog-execute!
    (SetValue pair (Predicate "MI Key")
      (CollectionOf (Type 'FormulaStream)
        (OrderedLink
          (ExecutionOutput
            (DefinedProcedure "dynamic MI")
            pair))))))

(install-mi (Concept "hello") (Concept "world"))

; Get MI (always current)
(define (get-mi A B)
  (cog-value-ref
    (cog-value (List (Concept A) (Concept B)) (Predicate "MI Key"))
    2))  ; Third element is the actual MI

(get-mi "hello" "world")

; MI updates automatically as more observations come in
(observe "hello" "world")
(get-mi "hello" "world")  ; New value

(observe "hello" "Gary")
(get-mi "hello" "world")  ; Updated again
```

## Best Practices

### 1. Always Attach Streams to Atoms

**DON'T** try to use streams in isolation:
```scheme
; ❌ WRONG - stream has no persistent location
(define my-stream (FormulaStream (Plus (Number 1) (Number 2))))
```

**DO** attach to well-known locations:
```scheme
; ✓ CORRECT - stream attached to anchor
(cog-execute!
  (SetValue (Anchor "my-anchor") (Predicate "stream-key")
    (CollectionOf (Type 'FormulaStream)
      (OrderedLink (Plus (Number 1) (Number 2))))))
```

### 2. Use FilterLink for Stream Pattern Matching

**DON'T** try to use MeetLink/QueryLink:
```scheme
; ❌ WRONG - MeetLink doesn't work on Values
(Meet (Variable "$x") <stream>)
```

**DO** use FilterLink with RuleLink:
```scheme
; ✓ CORRECT - FilterLink for stream processing
(Filter
  (Rule
    (Variable "$x")
    (Variable "$x")
    <rewrite>)
  <stream>)
```

### 3. Mark Stream End with VoidValue

**DON'T** leave streams without termination:
```scheme
; ❌ WRONG - infinite loop potential
(LinkValue (Item "a") (Item "b") (Item "c"))
```

**DO** terminate finite streams explicitly:
```scheme
; ✓ CORRECT - explicit end marker
(LinkValue
  (Item "a")
  (Item "b")
  (Item "c")
  (VoidValue))  ; End of stream
```

### 4. Use Appropriate Stream Type

**FormulaStream**: Numeric computations only (returns FloatValue)
```scheme
(FormulaStream (Plus (Number 1) (Number 2)))
```

**FutureStream**: Arbitrary Values
```scheme
(FutureStream (ValueOf (Concept "x") (Predicate "key")))
```

**FlatStream**: Sequential item extraction
```scheme
(FlatStream (OrderedLink <items>))
```

**SortedStream**: Ordered processing
```scheme
(SortedStream <sort-relation> <items>)
```

### 5. Use DrainLink for Complete Processing

**DON'T** manually loop over streams:
```scheme
; ❌ WRONG - manual iteration is error-prone
(cog-execute! stream)
(cog-execute! stream)
(cog-execute! stream)
; ... when does it end?
```

**DO** use DrainLink to exhaust streams:
```scheme
; ✓ CORRECT - automatic exhaustion
(cog-execute! (Drain processor))
```

### 6. Use ParallelLink for Long-Running Streams

**DON'T** block main thread on infinite streams:
```scheme
; ❌ WRONG - blocks forever
(cog-execute! (Drain infinite-stream-processor))
```

**DO** run in parallel for long-running processing:
```scheme
; ✓ CORRECT - background processing
(cog-execute! (ParallelLink (Drain processor)))
```

### 7. Understand LinkSignature vs CollectionOfLink

**LinkSignature**: Type conversions and Value construction
```scheme
; Convert types
(LinkSignature (Type 'Concept) (Predicate "foo"))

; Create streams in patterns
(LinkSignature (Type 'FlatStream) <source>)
```

**CollectionOfLink**: Wrap executables into streams
```scheme
; Create FormulaStream from formula
(CollectionOf (Type 'FormulaStream) <formula>)
```

## Common Patterns

### Pattern 1: Dynamic Metric Computation

```scheme
; Define metric formula
(DefineLink
  (DefinedProcedure "compute-metric")
  <formula-lambda>)

; Attach to target with FormulaStream
(cog-execute!
  (SetValue <target-atom> <metric-key>
    (CollectionOf (Type 'FormulaStream)
      (OrderedLink
        (ExecutionOutput
          (DefinedProcedure "compute-metric")
          <arguments>)))))
```

### Pattern 2: Stream Processing Pipeline

```scheme
; 1. Set up data source
(cog-set-value! <anchor> <source-key> <data>)

; 2. Create stream from source
(cog-execute!
  (SetValue <anchor> <stream-key>
    (CollectionOf (Type 'FlatStream) <data>)))

; 3. Define processor with FilterLink
(define processor
  (Filter
    (Rule <variables> <pattern> <rewrite>)
    (ValueOf <anchor> <stream-key>)))

; 4. Drain the stream
(cog-execute! (Drain processor))
```

### Pattern 3: Type Conversion in Stream

```scheme
(Filter
  (Rule
    (Variable "$item")
    (Variable "$item")

    ; Convert type and tag
    (Edge <tag-predicate>
      (LinkSignature (Type 'TargetType)
        (ValueOf (Variable "$item")))))

  <stream-source>)
```

### Pattern 4: Counting Stream Items

```scheme
(Filter
  (Rule
    (TypedVariable (Variable "$item") <type>)
    (Variable "$item")

    (LinkSignature (Type 'LinkValue)
      (IncrementValueOn (Variable "$item") <count-key> (Number 1))
      (IncrementValueOn <total-anchor> <total-key> (Number 1))))

  <stream>)
```

### Pattern 5: Sorted Priority Queue

```scheme
; Define sort relation
(define sort-fn
  (Lambda
    (VariableList (Variable "$l") (Variable "$r"))
    <comparison-expression>))

; Create sorted stream
(define queue
  (SortedStream sort-fn <items>))

; Process in order
(cog-value->list queue)
```

## Common Mistakes to Avoid

### Mistake 1: Trying to Store Streams Directly

❌ **WRONG**:
```scheme
; Cannot store stream as atom
(Concept "my-stream" (FormulaStream ...))
```

✅ **CORRECT**:
```scheme
; Attach to atom with SetValue
(cog-set-value! (Anchor "place") (Predicate "key")
  (FormulaStream ...))
```

### Mistake 2: Using MeetLink on Streams

❌ **WRONG**:
```scheme
; MeetLink doesn't work on Values
(Meet (Variable "$x") <stream>)
```

✅ **CORRECT**:
```scheme
; Use FilterLink for streams
(Filter (Rule ...) <stream>)
```

### Mistake 3: Forgetting VoidValue Terminator

❌ **WRONG**:
```scheme
; Finite stream without terminator
(LinkValue (Item "a") (Item "b"))
```

✅ **CORRECT**:
```scheme
; Explicit end marker
(LinkValue (Item "a") (Item "b") (VoidValue))
```

### Mistake 4: Not Using CollectionOfLink

❌ **WRONG**:
```scheme
; Cannot directly set executable as stream
(cog-set-value! atom key (ExecutionOutput ...))
```

✅ **CORRECT**:
```scheme
; Wrap with CollectionOfLink
(cog-execute!
  (SetValue atom key
    (CollectionOf (Type 'FormulaStream)
      (OrderedLink (ExecutionOutput ...)))))
```

### Mistake 5: Wrong Stream Type

❌ **WRONG**:
```scheme
; FormulaStream for non-numeric values
(FormulaStream (ValueOf (Concept "x") (Predicate "k")))
; Only works if value is FloatValue
```

✅ **CORRECT**:
```scheme
; FutureStream for arbitrary values
(FutureStream (ValueOf (Concept "x") (Predicate "k")))
```

### Mistake 6: Blocking on Infinite Streams

❌ **WRONG**:
```scheme
; Blocks forever in main thread
(cog-execute! (Drain infinite-processor))
```

✅ **CORRECT**:
```scheme
; Run in background thread
(cog-execute! (ParallelLink (Drain infinite-processor)))
```

## Summary

**Key Takeaways:**

1. **Streams are Values** - attach to Atoms, use FilterLink, not MeetLink
2. **FormulaStream** - numeric formulas that update automatically
3. **FutureStream** - wraps executables, returns arbitrary Values
4. **FlatStream** - serializes lists into sequential streams
5. **SortedStream** - maintains sorted order during streaming
6. **FilterLink** - pattern matching and transformation on streams
7. **LinkSignature** - type construction and conversion
8. **CollectionOfLink** - wraps executables into stream types
9. **DrainLink** - exhausts streams until VoidValue
10. **VoidValue** - marks end-of-stream

**Essential Patterns:**
- Attach streams to Atoms with SetValue
- Process streams with FilterLink + RuleLink
- Convert types with LinkSignature
- Create streams with CollectionOfLink
- Exhaust streams with DrainLink
- Run infinite streams with ParallelLink

**Remember:**
- Streams update dynamically on each access
- Use FilterLink (not MeetLink) for stream pattern matching
- Terminate finite streams with VoidValue
- Choose correct stream type for your use case
- Use DrainLink to ensure complete processing

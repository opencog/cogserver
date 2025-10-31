# Designing Structures in Atomese

This document contains notes that should be kept in mind when designing new data strutures in Atomese.

## CRITICAL: Atoms vs Values - The Fundamental Distinction

**This distinction is essential to understand how the AtomSpace works:**

### Atoms (Stored in AtomSpace)
* **Can be stored** in the AtomSpace
* **Have an Incoming Set** - a back-link index recording all atoms that contain them
* **Can be traversed** by the query engine (MeetLink, QueryLink, BindLink, GetLink)
* **Support graph traversal** - can walk upwards (via Incoming Set) and downwards (via Outgoing list)
* **Examples**: ConceptNode, PredicateNode, ListLink, EdgeLink, etc.
* **Structure**: Form trees and graphs through their connections
* **S-expression**: Have an s-expression format like `(Concept "cat")`

### Values (NOT Stored in AtomSpace)
* **Cannot be stored** in the AtomSpace directly
* **Do NOT have an Incoming Set** - no back-links, no indexing
* **Cannot be traversed** by the query engine (MeetLink/QueryLink don't work on Values)
* **Cannot support graph traversal** - no way to walk upwards through a Value hierarchy
* **Examples**: FloatValue, StringValue, LinkValue, Section, etc.
* **Structure**: Form trees, only.
* **S-expression**: Also have s-expression format like `(FloatValue 1.0 2.0 3.0)`

### Why This Matters

**The Query Engine** (MeetLink, QueryLink, BindLink, GetLink):
- Is a **recursive graph traversal engine**
- Walks graphs composed of trees, moving both upwards and downwards
- **Requires Atoms** because only Atoms have Incoming Sets

**FilterLink** (for Values):
- Is a **pattern matcher** not a traversal engine.
- Works on Value structures like LinkValue, Section, etc.
- Uses LinkSignature to describe Value types in patterns
- Travrses only trees, not graphs - simply matches tree patterns

### Common Confusion

**Both Atoms and Values use s-expressions**, which makes them look similar:
```
(ConceptNode "cat")           # This is an Atom
(FloatValue 1.0 2.0)          # This is a Value
(LinkValue (...))             # This is a Value
```

**But only Atoms can be queried with MeetLink/QueryLink!**
**Values require FilterLink for pattern matching.**

## CRITICAL: Global Uniqueness of Atoms

**Atoms are globally unique by their complete s-expression.**

### What Global Uniqueness Means

In the AtomSpace, if you create the same s-expression twice, you don't get two atoms - you get **the exact same atom instance**:

```scheme
(define cat1 (Concept "cat"))
(define cat2 (Concept "cat"))
;; cat1 and cat2 are THE SAME ATOM - same memory location
;; There is only ONE (Concept "cat") in the entire AtomSpace
```

This is fundamentally different from traditional databases where:
- Records are identified by row number or primary key
- Two records can have identical content but different identities
- Identity is separate from content

In AtomSpace:
- **Identity IS content**
- **Content IS address**
- Same s-expression anywhere = same atom instance
- Like Git objects (identified by SHA hash of content)
- Like string interning or hash-consing
- Content-addressable memory

### CRITICAL: Atomese vs Programming Languages

**Atomese is NOT Scheme, NOT Python, NOT any programming language.**

**Only Atomese can be stored in the AtomSpace.**

This distinction is fundamental:

**Atomese:**
- Both declarative AND functional
- Lives IN the AtomSpace
- Some atoms are literal constants: `(List (Concept "cat") (Concept "mat"))` - cannot be executed
- Some atoms are executable: `(Plus (Number 2) (Number 2))` - can execute to get `(Number 4)`
- The dual declarative/functional nature is fundamental to Atomese design

**Scheme/Python:**
- Imperative programming languages
- Run OUTSIDE the AtomSpace
- Manipulate atoms but don't live in the AtomSpace
- Have variables, procedures, execution flow
- Examples: `(define x ...)`, `for i in range(...)`, `if/then/else`

**What this means:**

When you write:
```scheme
(define my-atom (Concept "cat"))
```

- `(Concept "cat")` is Atomese - this DOES live in the AtomSpace
- `(define my-atom ...)` is Scheme - this does NOT live in the AtomSpace
- The Scheme variable `my-atom` is just a temporary handle to reference the atom from Scheme code
- The Atomese `(Concept "cat")` is permanent, globally unique, stored in the AtomSpace

**In Atomese, atoms reference other atoms by direct inclusion:**

```scheme
;; This is pure Atomese - all of this lives in the AtomSpace
(Edge (Predicate "eats")
  (List
    (Concept "cat")        ;; This atom is included directly
    (Concept "fish")))     ;; This atom is included directly

;; The Edge atom directly CONTAINS the Concept atoms
;; No variables, no pointers, no indirection
;; Just direct structural inclusion
```

**Why this matters for ID design:**

You cannot "store a reference" to an atom using Scheme variables or Python variables - those don't exist in the AtomSpace. In Atomese, you reference atoms by including them directly in other atoms' structure.

```scheme
;; ❌ WRONG thinking - trying to use IDs like database foreign keys
(Concept "cat-id-123")
(Edge (Predicate "eats") (List (Concept "cat-id-123") (Concept "fish")))
;; This treats "cat-id-123" as if it's a pointer, but it's just another concept!

;; ✓ CORRECT - direct structural inclusion
(Edge (Predicate "eats") (List (Concept "cat") (Concept "fish")))
;; The Concept "cat" is directly included in the Edge's structure
```

### Deep Implications

**1. Never use artificial ID numbers**

```scheme
;; ❌ WRONG - pointless artificial IDs
(Concept "sentence-001")
(Edge (Predicate "has-text")
  (List (Concept "sentence-001") (Concept "The cat sat on the mat")))

;; ✓ CORRECT - the text itself is globally unique
(Concept "The cat sat on the mat")
```

There is only ONE `(Concept "The cat sat on the mat")` in the entire AtomSpace, across all sessions, all time. The text itself uniquely identifies it. Adding "sentence-001" just creates indirection.

**Why IDs are harmful:**
- Adds indirection (must look up ID to find actual content)
- Wastes space (extra atoms for no purpose)
- Loses automatic deduplication
- Obscures the actual structure

**2. Automatic deduplication**

```scheme
;; Creating the same structure multiple times...
(Edge (Predicate "eats") (List (Concept "cat") (Concept "fish")))
(Edge (Predicate "eats") (List (Concept "cat") (Concept "fish")))
(Edge (Predicate "eats") (List (Concept "cat") (Concept "fish")))

;; ...results in only ONE atom in the AtomSpace
;; All three expressions point to the same memory location
```

**3. Structure creates identity**

```scheme
;; These are THREE DIFFERENT atoms (different structure):
(Edge (Predicate "found-in")
  (List (Concept "Common phrase") (Concept "file1.md")))
(Edge (Predicate "found-in")
  (List (Concept "Common phrase") (Concept "file2.md")))
(Edge (Predicate "found-in")
  (List (Concept "Common phrase") (Concept "file3.md")))

;; But all three SHARE the same (Concept "Common phrase") atom
;; The Edge atoms differ because their full s-expression differs
```

**4. Direct structural navigation - atoms reference other atoms by including them**

```scheme
;; ✓ Link atoms directly to each other
(Edge (Predicate "parse-of")
  (List
    (Concept "The cat sat")
    (Set
      (Edge (Bond "Ss*s") (List (Word "cat") (Word "sat")))
      (Edge (Bond "Dsu") (List (Word "the") (Word "cat"))))))

;; The Concept atom is globally unique - use it directly
;; No IDs, no lookups, no indirection
```

### Handling Metadata Without IDs

If you need to annotate an atom with metadata, attach it directly:

```scheme
;; ❌ WRONG - creating ID to attach metadata
(Concept "edge-id-123")
(Link (Concept "edge-id-123") (Edge ...))
(Metadata (Concept "edge-id-123") (Number 0.95))

;; ✓ CORRECT - attach metadata directly to the atom using Values
(SetValue
  (Edge (Predicate "serves") (List (Concept "implementation") (Concept "goal")))
  (Predicate "confidence")
  (Number 0.95))
```

The edge atom itself is globally unique - use it directly as the key for metadata.

### When You Think You Need an ID

**Ask yourself:**
1. Is this atom's s-expression already globally unique? (Almost always yes)
2. Am I trying to distinguish different contexts? → Include context IN the structure
3. Do I need metadata? → Use Values (`SetValue`) or wrap in metadata atom
4. Am I creating indirection for no reason? → Use atoms directly

### Checklist for Designing New Structures

Before adding any kind of ID field:
1. **Does this atom's structure already make it unique?** (Usually yes)
2. **Am I trying to distinguish different contexts?** → Include context in the structure
3. **Do I need metadata?** → Use Values or wrap in metadata atom
4. **Am I creating indirection for no reason?** → Use atoms directly

**Mental model:** Think of AtomSpace like:
- Git repository: objects identified by content hash
- String interning: same string = same pointer
- Hash-consing: structure sharing via content addressing

**The s-expression IS the universal ID. Nothing else is needed.**

## SetLink vs ListLink: Ordering and Search Performance

**A critical design decision when storing collections of atoms.**

### The Fundamental Difference

**ListLink:**
- **Ordered** - elements have a defined sequence
- Pattern matching respects order
- Example: `(List (Concept "first") (Concept "second") (Concept "third"))`

**SetLink:**
- **Unordered** - no defined sequence for elements
- Pattern matching considers all permutations
- Example: `(Set (Concept "A") (Concept "B") (Concept "C"))`

### Performance Implications

**SetLink causes combinatorial explosion during pattern matching:**

When you write a pattern like:
```scheme
(Set (Variable "$x") (Variable "$y") (Glob "$rest"))
```

The pattern matcher must explore **all N-factorial permutations** of the SetLink:
- 3 elements: 6 permutations
- 4 elements: 24 permutations
- 5 elements: 120 permutations
- 10 elements: 3,628,800 permutations

This affects:
- **QueryLink/MeetLink**: Must try all permutations to find matches
- **FilterLink**: Must explore permutations for pattern matching
- **CPU time**: Can become very expensive for large sets

**ListLink avoids permutation explosion but requires complex Glob patterns:**

To find two elements at unknown positions in a List:
```scheme
;; Need this pattern
(List
  (Glob "$front")           ;; stuff before X
  (Variable "$x")
  (Glob "$middle")          ;; stuff between X and Y
  (Variable "$y")
  (Glob "$end"))            ;; stuff after Y

;; AND also need the reversed pattern
(List
  (Glob "$front")
  (Variable "$y")           ;; Y before X
  (Glob "$middle")
  (Variable "$x")
  (Glob "$end"))
```

Glob patterns also cause combinatorial search (trying all possible ways to split the list), but typically less severe than N-factorial permutations.

### When to Use SetLink

**Use SetLink when:**
- Order genuinely doesn't matter semantically
- You need position-independent matching
- Set size is small (< 10 elements typically)
- You're willing to pay the permutation cost for simpler patterns

**Example use case:**
```scheme
;; Storing all bonds from a sentence parse
(Edge (Predicate "parse-of")
  (List
    (Concept "The cat sat")
    (Set
      (Edge (Bond "Ss*s") (List (Word "cat") (Word "sat")))
      (Edge (Bond "Dsu") (List (Word "the") (Word "cat")))
      ;; ... more bonds
    )))
```

Here SetLink makes sense because:
- Bond order in the parse is semantically meaningless
- Queries like "find sentences with Ss*s and Os bonds" don't care about order
- Avoids needing multiple Glob patterns for different bond positions

**But be aware:** If a sentence has 20 bonds, pattern matching will explore permutations of those 20 elements.

### When to Use ListLink

**Use ListLink when:**
- Order matters semantically (sequences, temporal ordering)
- Set size is large (> 10 elements)
- Performance is critical
- You can write explicit positional patterns

**Example use case:**
```scheme
;; Temporal sequence - order matters
(List
  (Event "wake-up")
  (Event "eat-breakfast")
  (Event "go-to-work"))
```

### The Tradeoff

**SetLink:**
- ✓ Simpler query patterns (position-independent)
- ✗ N-factorial permutation explosion in pattern matching

**ListLink:**
- ✓ No permutation explosion
- ✗ Complex Glob patterns for position-independent matching
- ✗ Need multiple patterns for different orderings

**Choose based on:**
1. Does order matter semantically?
2. How large is the collection?
3. What queries will you run?
4. What performance cost can you tolerate?

**There is no perfect answer** - both have tradeoffs. Choose consciously based on your use case.


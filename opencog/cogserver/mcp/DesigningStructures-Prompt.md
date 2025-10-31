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


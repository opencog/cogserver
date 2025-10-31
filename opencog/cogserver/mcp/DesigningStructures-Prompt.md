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


# Pattern Matching and Queries in AtomSpace

This prompt explains how to use pattern matching to search the AtomSpace for **Atoms** that match specific patterns.

## CRITICAL: This is for Atoms Only

**The query engine (MeetLink, QueryLink, BindLink, GetLink) works ONLY on Atoms.**

### Atoms vs Values

**Atoms** (what these tools work on):
- Stored in the AtomSpace
- Have Incoming Sets (back-links for graph traversal)
- Can be queried with MeetLink/QueryLink
- Examples: ConceptNode, PredicateNode, ListLink, EdgeLink

**Values** (these tools do NOT work on):
- NOT stored in the AtomSpace
- NO Incoming Sets - cannot be traversed
- Cannot be queried with MeetLink/QueryLink
- **Use FilterLink instead** for pattern matching on Values
- Examples: FloatValue, StringValue, LinkValue, Section

**The query engine is a recursive graph traversal engine** that walks the graph both upwards (via Incoming Sets) and downwards (via Outgoing lists). Since Values don't have Incoming Sets, they cannot be traversed by the query engine.

**For pattern matching on Values, see the "Working with Values" resource (atomspace://docs/working-with-values) for FilterLink documentation.**

## Overview: The Four Query Link Types

The AtomSpace provides four link types for pattern matching queries on Atoms. **Two are modern and recommended, two are deprecated:**

### ✅ RECOMMENDED: Use These

1. **MeetLink** - Find atoms matching a pattern
   - Returns: `QueueValue` (thread-safe, no AtomSpace pollution)
   - Use when: You want to find which atoms satisfy a pattern
   - Does NOT perform graph rewriting

2. **QueryLink** - Find matches AND rewrite/transform them
   - Returns: `QueueValue` (thread-safe, no AtomSpace pollution)
   - Use when: You want to transform matched results
   - DOES perform graph rewriting with the third argument

### ❌ DEPRECATED: Avoid These

3. **GetLink** - Old version of MeetLink
   - Returns: `SetLink` (pollutes AtomSpace, requires manual cleanup)
   - Problem: Creates SetLinks that clutter the AtomSpace
   - Replacement: Use `MeetLink` instead

4. **BindLink** - Old version of QueryLink
   - Returns: `SetLink` (pollutes AtomSpace, requires manual cleanup)
   - Problem: Creates SetLinks that clutter the AtomSpace
   - Replacement: Use `QueryLink` instead

## When to Use Which Link Type

### Use MeetLink When:
- Finding atoms that match a pattern
- Searching for variable groundings
- Checking what satisfies certain conditions
- You DON'T need to transform/rewrite results

### Use QueryLink When:
- You need to transform matched results
- Applying rules or rewrites
- Creating new atoms based on matches
- You want both pattern matching AND graph rewriting

### Never Use GetLink or BindLink:
- They create unnecessary SetLinks in the AtomSpace
- Slower and less efficient
- Deprecated in modern OpenCog

## MeetLink: Finding Pattern Matches

### Basic Syntax

**Two-argument form (most common):**
```scheme
(MeetLink
   <variable-declarations>
   <pattern-to-match>)
```

**Example: Find all people interested in psychology**
```scheme
(MeetLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
```

### Variable Declarations

**Simple variable:**
```scheme
(Variable "$x")
```

**Typed variable:**
```scheme
(TypedVariable (Variable "$person") (Type "ConceptNode"))
```

**Multiple variables:**
```scheme
(VariableList
   (Variable "$person")
   (Variable "$topic"))
```

**Multiple typed variables:**
```scheme
(VariableList
   (TypedVariable (Variable "$person") (Type "ConceptNode"))
   (TypedVariable (Variable "$topic") (Type "ConceptNode")))
```

### Pattern Clauses

**Single pattern:**
```scheme
(Edge (Predicate "likes") (List (Variable "$x") (Concept "pizza")))
```

**Multiple patterns with AndLink:**
```scheme
(And
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology")))
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "programming"))))
```

**This finds people interested in BOTH psychology AND programming.**

### Complete MeetLink Examples

**Example 1: Find all ConceptNodes**
```scheme
(MeetLink
   (TypedVariable (Variable "$x") (Type "ConceptNode"))
   (Variable "$x"))
```

**Example 2: Find people with any interest**
```scheme
(MeetLink
   (VariableList (Variable "$person") (Variable "$topic"))
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Variable "$topic"))))
```

**Example 3: Find experts (people interested in 3+ topics)**
```scheme
(MeetLink
   (Variable "$person")
   (And
      (Edge (Predicate "interested-in")
            (List (Variable "$person") (Variable "$topic1")))
      (Edge (Predicate "interested-in")
            (List (Variable "$person") (Variable "$topic2")))
      (Edge (Predicate "interested-in")
            (List (Variable "$person") (Variable "$topic3")))))
```

## QueryLink: Pattern Matching with Rewriting

### Critical Difference from MeetLink

**QueryLink has THREE arguments, not two:**
```scheme
(QueryLink
   <variable-declarations>
   <pattern-to-match>
   <rewrite-template>)
```

**The third argument is the rewrite template that creates the output!**

### Basic Syntax

```scheme
(QueryLink
   (Variable "$x")
   (Edge (Predicate "likes") (List (Variable "$x") (Concept "pizza")))
   (Concept "pizza-lover"))
```

This finds all $x that like pizza, and returns `(Concept "pizza-lover")` for each match.

### Common Mistake (That You Made!)

❌ **WRONG - Two arguments:**
```scheme
(QueryLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
```

**This returns empty results because there's no rewrite template (third argument)!**

✅ **CORRECT - Three arguments:**
```scheme
(QueryLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
   (Variable "$person"))
```

**The third argument `(Variable "$person")` tells QueryLink to return the person.**

### QueryLink vs MeetLink Comparison

**To find people interested in psychology:**

**Using MeetLink (simpler, recommended):**
```scheme
(MeetLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
```

**Using QueryLink (more complex, same result):**
```scheme
(QueryLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
   (Variable "$person"))
```

**When QueryLink is actually useful - Creating derived atoms:**
```scheme
(QueryLink
   (Variable "$person")
   (Edge (Predicate "interested-in")
         (List (Variable "$person") (Concept "psychology"))))
   (Edge (Predicate "is-psychologist")
         (List (Variable "$person"))))
```

**This finds people interested in psychology and creates new edges marking them as psychologists.**

### Rewrite Templates

The third argument can be any Atomese expression using the matched variables:

**Return the variable itself:**
```scheme
(Variable "$x")
```

**Create a new structure:**
```scheme
(List (Concept "found") (Variable "$x"))
```

**Build new relationships:**
```scheme
(Edge (Predicate "new-property")
      (List (Variable "$x") (Concept "value")))
```

**Transform multiple variables:**
```scheme
(List (Variable "$person") (Variable "$topic") (Concept "expertise"))
```

## Using AndLink in Patterns

### Find Atoms Matching ALL Conditions

```scheme
(MeetLink
   (Variable "$person")
   (And
      (Edge (Predicate "interested-in")
            (List (Variable "$person") (Concept "psychology")))
      (Edge (Predicate "interested-in")
            (List (Variable "$person") (Concept "programming")))))
```

**This finds people interested in BOTH psychology AND programming.**

### Multiple Variables with AndLink

```scheme
(MeetLink
   (VariableList (Variable "$person") (Variable "$friend"))
   (And
      (Edge (Predicate "knows")
            (List (Variable "$person") (Variable "$friend")))
      (Edge (Predicate "interested-in")
            (List (Variable "$friend") (Concept "AI")))))
```

**This finds pairs of ($person, $friend) where $person knows $friend and $friend is interested in AI.**

### Chaining Conditions

```scheme
(MeetLink
   (VariableList (Variable "$x") (Variable "$y") (Variable "$z"))
   (And
      (Edge (Predicate "parent-of") (List (Variable "$x") (Variable "$y")))
      (Edge (Predicate "parent-of") (List (Variable "$y") (Variable "$z")))))
```

**This finds grandparent relationships: $x is grandparent of $z through $y.**

## Executing Queries with MCP Tools

### Using the execute Tool

**Tool**: `execute`

**For MeetLink:**
```json
{
  "atomese": "(MeetLink (Variable \"$x\") (Edge (Predicate \"likes\") (List (Variable \"$x\") (Concept \"pizza\"))))"
}
```

**For QueryLink (remember 3 arguments!):**
```json
{
  "atomese": "(QueryLink (Variable \"$x\") (Edge (Predicate \"likes\") (List (Variable \"$x\") (Concept \"pizza\"))) (Variable \"$x\"))"
}
```

### Understanding Results

**MeetLink returns QueueValue (shown as UnisetValue):**
```scheme
(UnisetValue (LinkValue (ConceptNode "Alice"))(LinkValue (ConceptNode "Bob")))
```

**This means:** Found 2 matches: Alice and Bob

**QueryLink returns QueueValue:**
```scheme
(UnisetValue (LinkValue (ConceptNode "Alice"))(LinkValue (ConceptNode "Bob")))
```

**With rewrite, might return transformed results:**
```scheme
(UnisetValue
   (LinkValue (EdgeLink (PredicateNode "is-psychologist")(ListLink (ConceptNode "Alice"))))
   (LinkValue (EdgeLink (PredicateNode "is-psychologist")(ListLink (ConceptNode "Bob")))))
```

## Complete Working Examples

### Example 1: Find All Concepts (MeetLink)

**Create test data:**
```json
{
  "atomese": "(Concept \"apple\")"
}
{
  "atomese": "(Concept \"banana\")"
}
{
  "atomese": "(Concept \"cherry\")"
}
```

**Query:**
```json
{
  "atomese": "(MeetLink (TypedVariable (Variable \"$x\") (Type \"ConceptNode\")) (Variable \"$x\"))"
}
```

**Result:**
```scheme
(UnisetValue
   (LinkValue (ConceptNode "apple"))
   (LinkValue (ConceptNode "banana"))
   (LinkValue (ConceptNode "cherry")))
```

### Example 2: Find Shared Interests (MeetLink with AndLink)

**Create test data:**
```json
{"atomese": "(Concept \"Alice\")"}
{"atomese": "(Concept \"Bob\")"}
{"atomese": "(Concept \"Python\")"}
{"atomese": "(Concept \"AI\")"}
{"atomese": "(Predicate \"interested-in\")"}
{"atomese": "(Edge (Predicate \"interested-in\") (List (Concept \"Alice\") (Concept \"Python\")))"}
{"atomese": "(Edge (Predicate \"interested-in\") (List (Concept \"Alice\") (Concept \"AI\")))"}
{"atomese": "(Edge (Predicate \"interested-in\") (List (Concept \"Bob\") (Concept \"Python\")))"}
{"atomese": "(Edge (Predicate \"interested-in\") (List (Concept \"Bob\") (Concept \"AI\")))"}
```

**Query - Find people interested in BOTH Python AND AI:**
```json
{
  "atomese": "(MeetLink (Variable \"$person\") (And (Edge (Predicate \"interested-in\") (List (Variable \"$person\") (Concept \"Python\"))) (Edge (Predicate \"interested-in\") (List (Variable \"$person\") (Concept \"AI\")))))"
}
```

**Result:**
```scheme
(UnisetValue
   (LinkValue (ConceptNode "Alice"))
   (LinkValue (ConceptNode "Bob")))
```

### Example 3: Create Derived Facts (QueryLink)

**Query - Mark Python+AI enthusiasts:**
```json
{
  "atomese": "(QueryLink (Variable \"$person\") (And (Edge (Predicate \"interested-in\") (List (Variable \"$person\") (Concept \"Python\"))) (Edge (Predicate \"interested-in\") (List (Variable \"$person\") (Concept \"AI\")))) (Edge (Predicate \"is-expert\") (List (Variable \"$person\") (Concept \"AI-programming\"))))"
}
```

**Result - New edges created:**
```scheme
(UnisetValue
   (LinkValue (EdgeLink (PredicateNode "is-expert")(ListLink (ConceptNode "Alice")(ConceptNode "AI-programming"))))
   (LinkValue (EdgeLink (PredicateNode "is-expert")(ListLink (ConceptNode "Bob")(ConceptNode "AI-programming")))))
```

## Best Practices

### ✅ Do This

1. **Use MeetLink for simple searches**
   - Faster, cleaner, easier to understand
   - No unnecessary rewriting overhead

2. **Use QueryLink only when you need rewriting**
   - Creating derived facts
   - Transforming matched patterns
   - Always provide the third argument!

3. **Use AndLink for multiple conditions**
   - Finds atoms matching ALL conditions
   - More efficient than separate queries

4. **Type your variables when possible**
   - Faster query execution
   - Prevents matching wrong types
   - `(TypedVariable (Variable "$x") (Type "ConceptNode"))`

5. **Check atom counts before querying**
   - Use `reportCounts` first
   - Large result sets can be slow

### ❌ Avoid This

1. **Don't use GetLink or BindLink**
   - Deprecated, pollutes AtomSpace
   - Use MeetLink or QueryLink instead

2. **Don't forget QueryLink's third argument**
   - QueryLink needs: variable, pattern, **rewrite**
   - Without rewrite, you get empty results

3. **Don't use QueryLink when MeetLink would work**
   - QueryLink is heavier due to rewriting
   - Use MeetLink unless you need transformation

4. **Don't create unbounded queries**
   - Always constrain variables when possible
   - Use type constraints
   - Check counts first with `reportCounts`

## Quick Reference

| Link Type | Arguments | Returns | Use When |
|-----------|-----------|---------|----------|
| **MeetLink** ✅ | 2: variables, pattern | QueueValue | Finding matches |
| **QueryLink** ✅ | 3: variables, pattern, rewrite | QueueValue | Finding + transforming |
| GetLink ❌ | 2: variables, pattern | SetLink | NEVER (use MeetLink) |
| BindLink ❌ | 3: variables, pattern, rewrite | SetLink | NEVER (use QueryLink) |

## Common Errors and Solutions

### Error: QueryLink returns empty UnisetValue

**Problem:** Only provided 2 arguments to QueryLink

**Solution:** Add the third argument (rewrite template)

❌ Wrong:
```scheme
(QueryLink (Variable "$x") (pattern...))
```

✅ Correct:
```scheme
(QueryLink (Variable "$x") (pattern...) (Variable "$x"))
```

### Error: Too many results returned

**Problem:** Unbounded query matches too many atoms

**Solution:** Add type constraints or more specific patterns

❌ Wrong:
```scheme
(MeetLink (Variable "$x") (Variable "$x"))
```

✅ Correct:
```scheme
(MeetLink
   (TypedVariable (Variable "$x") (Type "ConceptNode"))
   (Edge (Predicate "specific-property") (List (Variable "$x") ...)))
```

### Error: AndLink not finding matches

**Problem:** Pattern too restrictive or atoms don't exist

**Solution:** Test each condition separately first

```scheme
; Test individually:
(MeetLink (Variable "$x") (pattern1))
(MeetLink (Variable "$x") (pattern2))

; Then combine:
(MeetLink (Variable "$x") (And (pattern1) (pattern2)))
```

## Summary

- **MeetLink**: Find matches (2 arguments)
- **QueryLink**: Find + transform (3 arguments)
- **AndLink**: Match ALL conditions
- **GetLink/BindLink**: Never use (deprecated)
- **Always** provide QueryLink's rewrite argument
- **Prefer** MeetLink unless you need transformation

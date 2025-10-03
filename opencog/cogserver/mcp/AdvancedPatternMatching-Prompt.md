# Advanced AtomSpace Pattern Matching

*A comprehensive guide to sophisticated pattern matching in the OpenCog AtomSpace*

---

## Overview

This document covers four advanced query constructs that enable powerful pattern matching beyond basic `MeetLink` and `QueryLink`:

1. **AbsentLink** - Ensures results do NOT match a particular pattern
2. **ChoiceLink** - Matches any of several alternative patterns
3. **AlwaysLink** - Requires a clause to be true for ALL instances ("for-all" semantics)
4. **GroupLink** - Groups results by shared variables (similar to SQL `GROUP BY`)

All examples in this guide have been tested against a live AtomSpace and can be executed via the MCP interface.

---

## 1. AbsentLink - Pattern Negation

### Purpose
The `AbsentLink` evaluates to **true** when the specified pattern is **NOT present** in the AtomSpace. It implements intuitionistic logic's "unknown" or "don't know" state.

### Key Characteristics

- **Evaluates to true**: When the wrapped pattern cannot be found in the AtomSpace
- **Variables leak out**: Unlike some other link types, variables in `AbsentLink` are not bound and remain available to the broader query context
- **Intuitionistic logic**: Represents "absence of knowledge" rather than "known to be false"
- **Different from NotLink**: `AbsentLink` tests pattern absence; `NotLink` tests boolean falsity

### Conceptual Model

Think of the AtomSpace as a Kripke frame containing all currently known facts. `AbsentLink` allows you to query: *"Is this fact currently unknown?"* This implements the Law of the Excluded Middle (LEM) or reductio ad absurdum (RAA) for knowledge representation.

### Basic Example

```scheme
; Find fruits that do NOT have red color
(MeetLink
  (VariableList (Variable "$fruit") (Variable "$color"))
  (And
    (Evaluation (Predicate "color")
      (List (Variable "$fruit") (Variable "$color")))
    (Absent
      (Evaluation (Predicate "color")
        (List (Variable "$fruit") (Concept "red"))))))
```

**Explanation**: This query finds all fruit-color pairs where the fruit is **not** red. The `Absent` clause ensures no `(color $fruit red)` relationship exists.

**Tested Result**: Returns `(banana, yellow)` when the AtomSpace contains:
- `(color apple red)`
- `(color apple green)`
- `(color banana yellow)`

### Advanced Example: State Detection

```scheme
; Check if room is empty (no visible items)
(QueryLink
  (Variable "$x")
  (Absent
    (Evaluation (Predicate "visibility")
      (List (Variable "$x"))))
  (Put
    (State (Anchor "Room State") (Variable "$x"))
    (Concept "room empty")))
```

**Pattern**: If no visibility relationships exist, set room state to empty.

### Common Use Cases

1. **Negative constraints**: "Find X that is NOT Y"
2. **Absence detection**: "Is there anything matching this pattern?"
3. **Exception handling**: "Results except those matching..."
4. **State validation**: "Ensure condition does not hold"

### Important Notes

- The `Absent` clause must be wrapped in a pattern matcher context (`MeetLink`, `QueryLink`)
- Variables inside `AbsentLink` can be used elsewhere in the query
- Evaluates at query time, not at link creation time
- Combines naturally with `And`, `Or`, and other logical connectives

### Pattern: Find items without a specific property

```scheme
(MeetLink
  (Variable "$item")
  (And
    ; Item exists with some property
    (Evaluation (Predicate "has-property")
      (List (Variable "$item") (Variable "$prop")))
    ; But NOT this specific property
    (Absent
      (Evaluation (Predicate "has-property")
        (List (Variable "$item") (Concept "toxic"))))))
```

---

## 2. ChoiceLink - Pattern Alternatives

### Purpose
The `ChoiceLink` allows pattern matching to succeed if **any one** of several alternative patterns is found. It implements logical OR for structural pattern matching.

### Key Characteristics

- **Disjunctive matching**: Succeeds if any choice matches
- **Variables leak out**: Variables used in choices are available to outer context
- **Type theory analogue**: Corresponds to "sum types" or "disjoint unions"
- **Top-level placement**: Must appear at top of the pattern structure (see constraints below)

### Basic Example

```scheme
; Find fruit that is either red or yellow
(MeetLink
  (Variable "$fruit")
  (Choice
    (Evaluation (Predicate "color")
      (List (Variable "$fruit") (Concept "red")))
    (Evaluation (Predicate "color")
      (List (Variable "$fruit") (Concept "yellow")))))
```

**Tested Result**: Returns `apple` and `banana` (both fruits matching either color).

### Critical Constraint: Top-Level Placement

**This WILL NOT work:**
```scheme
; WRONG - ChoiceLink nested inside pattern
(MeetLink
  (Variable "$fruit")
  (Evaluation (Predicate "color")
    (List
      (Variable "$fruit")
      (Choice                     ; ← Cannot use Choice here
        (Concept "red")
        (Concept "green")))))
```

**This WILL work:**
```scheme
; CORRECT - ChoiceLink at top level
(MeetLink
  (Variable "$fruit")
  (Choice
    (Evaluation (Predicate "color")
      (List (Variable "$fruit") (Concept "red")))
    (Evaluation (Predicate "color")
      (List (Variable "$fruit") (Concept "green")))))
```

**Explanation**: `ChoiceLink` must appear at the top of the pattern structure. Each alternative must be a complete pattern clause.

### Multiple Choices Example

```scheme
; Find items that are EITHER:
; - Sweet-tasting, OR
; - Red or yellow colored, OR
; - Members of the fruit category
(MeetLink
  (Variable "$item")
  (Choice
    (Evaluation (Predicate "taste")
      (List (Variable "$item") (Concept "sweet")))
    (Evaluation (Predicate "color")
      (List (Variable "$item") (Concept "red")))
    (Evaluation (Predicate "color")
      (List (Variable "$item") (Concept "yellow")))
    (Inheritance (Variable "$item") (Concept "fruit"))))
```

### Combining Choice with Other Links

```scheme
; Find fruits that are sweet AND (red OR yellow)
(MeetLink
  (Variable "$fruit")
  (And
    (Evaluation (Predicate "taste")
      (List (Variable "$fruit") (Concept "sweet")))
    (Choice
      (Evaluation (Predicate "color")
        (List (Variable "$fruit") (Concept "red")))
      (Evaluation (Predicate "color")
        (List (Variable "$fruit") (Concept "yellow"))))))
```

**Pattern**: The `And` requires sweetness, and the nested `Choice` allows either red or yellow.

### Common Use Cases

1. **Multi-criteria search**: "Find items matching any of these criteria"
2. **Type alternatives**: "Match either type A or type B"
3. **Fallback patterns**: "Try this pattern, or this one, or that one"
4. **Category expansion**: "Search across multiple related categories"

### Important Notes

- **Not recommended with NotLink**: Combining `Choice` and `Not` can lead to unexpected behavior
- **Variable consistency**: Same variable name across choices will unify
- **Returns union**: All matches from all choices are combined in result set

---

## 3. AlwaysLink - Universal Quantification

### Purpose
The `AlwaysLink` implements **"for-all" semantics**, ensuring that a condition holds for **every instance** found during pattern matching. It's the universal quantifier (∀) of the AtomSpace.

### Key Characteristics

- **Universal quantification**: Pattern must match for ALL instances
- **Variables remain free**: Unlike `ForAllLink`, variables are not bound
- **Early termination**: Search can halt as soon as any instance fails
- **Composable**: Can be combined with other patterns sharing variables
- **Single grouping constraint**: Demands "one and only one group" where condition holds

### Conceptual Model

While `Present` asks "does at least one instance exist?", `AlwaysLink` asks "do ALL instances satisfy this condition?" It's the difference between existential (∃) and universal (∀) quantification.

### Basic Example

```scheme
; Find baskets containing ONLY red balls
(MeetLink
  (VariableList (Variable "$basket") (Variable "$ball"))
  (And
    (Member (Variable "$ball") (Variable "$basket"))
    (Always
      (Evaluation (Predicate "is_red")
        (Variable "$ball")))))
```

**Explanation**:
1. For each basket, examine all balls
2. The `Always` clause requires **every** ball to satisfy `is_red`
3. Returns only baskets where no non-red ball exists

**Tested Result**: Returns `basket_A` (containing only red balls), but **not** `basket_B` (which contains green_ball).

### Advanced Example: Pairwise Comparison

```scheme
; Find baskets where all balls have the same color
(MeetLink
  (VariableList
    (Variable "$basket")
    (Variable "$some_ball")
    (Variable "$other_ball")
    (Variable "$some_color")
    (Variable "$other_color"))
  (And
    ; Pick out two balls from the same basket
    (Member (Variable "$some_ball") (Variable "$basket"))
    (Member (Variable "$other_ball") (Variable "$basket"))

    ; Get their colors
    (Evaluation (Variable "$some_color") (Variable "$some_ball"))
    (Evaluation (Variable "$other_color") (Variable "$other_ball"))

    ; ALWAYS ensure colors match
    (Always
      (Equal (Variable "$some_color") (Variable "$other_color")))))
```

**Pattern**: For every pair of balls in a basket, their colors must match.

### Relationship to GroupLink

`AlwaysLink` can be thought of as a special case of `GroupLink` where you demand exactly one group. From the examples:

> "GroupLink is a 'local' version of AlwaysLink. The AlwaysLink asks that all search results must have in common the specified clause, or equivalently, that there must be one and only one group."

### Common Use Cases

1. **Homogeneity checking**: "All members have same property"
2. **Constraint validation**: "Every instance satisfies constraint"
3. **Purity testing**: "Container holds only items of type X"
4. **Consistency verification**: "All related items match criteria"

### Alternative Implementation (Less Efficient)

You can achieve similar results with nested queries and `NotLink`, but it's less efficient:

```scheme
; Find baskets WITHOUT non-red balls (double negative)
(MeetLink
  (Variable "$basket")
  (And
    (Inheritance (Variable "$basket") (Concept "basket"))
    (NotLink
      (SatisfactionLink (Variable "$ball")
        (And
          (Member (Variable "$ball") (Variable "$basket"))
          (Absent
            (Evaluation (Predicate "is_red")
              (Variable "$ball"))))))))
```

**Why less efficient**:
- Requires nested query setup and execution
- Two-pass search (outer and inner)

### Important Notes

- **Early termination**: Pattern matcher can stop as soon as any instance fails the `Always` condition
- **Free variables**: Variables in `AlwaysLink` remain available to outer context
- **Scope**: The "always" condition applies to all groundings of the specified variables

---

## 4. GroupLink - Result Grouping

### Purpose
The `GroupLink` groups query results by shared variable values, similar to SQL's `GROUP BY` clause. It creates "kernels" around which similar results cluster.

### Key Characteristics

- **SQL-like grouping**: Groups results sharing common variable groundings
- **Hypergraph perspective**: Creates connected sub-graphs through shared atoms
- **Local for-all**: Each group satisfies a local "for-all" condition
- **Size constraints**: Can filter groups by member count
- **Rewrite context**: Grouped results can be transformed via QueryLink rewrite clause

### Conceptual Models

Three ways to think about `GroupLink`:

1. **Connected hypergraphs**: All results in a group are connected through a shared atom (the grouping kernel)
2. **Kernel clustering**: Results cluster around a common "kernel" atom
3. **Local for-all**: Within each group, all members share the grouping property

### Basic Example

```scheme
; Group balls by their basket
(QueryLink
  (VariableList (Variable "$basket") (Variable "$ball"))
  (And
    (Member (Variable "$ball") (Variable "$basket"))
    (Group (Variable "$basket")))
  (Variable "$ball"))
```

**Tested Result**:
```
(UnisetValue
  (LinkValue (Concept "green_ball") (Concept "red_ball_3"))  ; basket_B group
  (LinkValue (Concept "red_ball_2") (Concept "red_ball_1")))  ; basket_A group
```

**Explanation**: Results are partitioned into two groups, one for each basket. All balls in the same basket form a connected group.

### Example with Rewrite

```scheme
; Group properties by category, create structured output
(QueryLink
  (VariableList (Variable "$item") (Variable "$category"))
  (And
    (Present
      (Edge (Predicate "property")
        (List (Variable "$item") (Variable "$category"))))
    (Group (Variable "$category")))

  ; Rewrite: create Implication links showing grouping
  (Evaluation (Concept "things that go together")
    (Implication (Variable "$category") (Variable "$item"))))
```

**Pattern**: Find all properties, group by category, and create a structured representation of each group.

### Size Constraints with IntervalLink

```scheme
; Find groups with 2-5 members
(QueryLink
  (VariableList (Variable "$X") (Variable "$Y"))
  (And
    (Present
      (Edge (Predicate "property")
        (List (Variable "$X") (Variable "$Y"))))
    (Group
      (Variable "$Y")
      (Interval (Number 2) (Number 5))))
  (Variable "$X"))
```

**Important**:
- Lower bound: minimum group size (2)
- Upper bound: maximum group size (5), or -1 for unlimited
- Constraint applies **before** the rewrite clause, not after

### Collapsed Groupings

```scheme
; Return just the group identifiers (category names)
(QueryLink
  (VariableList (Variable "$item") (Variable "$category"))
  (And
    (Present
      (Edge (Predicate "property")
        (List (Variable "$item") (Variable "$category"))))
    (Group
      (Variable "$category")
      (Interval (Number 2) (Number -1))))

  ; Return only the category (collapses each group to one item)
  (Variable "$category"))
```

**Result**: Instead of all items in each group, returns one category name per group (groups with 2+ members).

### Relationship to AlwaysLink

From the documentation:

> "GroupLink is a 'local' version of AlwaysLink. The AlwaysLink asks that all search results must have in common the specified clause, or equivalently, that there must be one and only one group. The GroupLink relaxes this demand for there to be only one, and presents several groupings, as these occur."

**Key insight**:
- `AlwaysLink` = demanding exactly 1 group
- `GroupLink` = allowing multiple groups

### Common Use Cases

1. **Clustering**: "Group similar items together"
2. **Categorization**: "Partition results by category"
3. **Aggregation**: "Collect related facts" (combine with size constraints)
4. **Network analysis**: "Find connected components"
5. **Statistical queries**: "Count members per group"

### Important Notes

- **Multiple grouping variables**: Can group by tuples of variables
- **Disjoint groups**: Groups don't share the grouping kernel atom
- **May connect otherwise**: Groups can be connected through other relationships
- **Size before rewrite**: `IntervalLink` constraints apply to group size before the rewrite clause executes
- **Result format**: Returns `UnisetValue` or `QueueValue` containing grouped results

---

## Practical Query Patterns

### Pattern 1: Exclusion with AbsentLink

**Problem**: Find users who have NOT completed a specific task.

```scheme
(MeetLink
  (Variable "$user")
  (And
    (Evaluation (Predicate "is-user") (Variable "$user"))
    (Absent
      (Evaluation (Predicate "completed")
        (List (Variable "$user") (Concept "task-123"))))))
```

### Pattern 2: Multi-Option Search with ChoiceLink

**Problem**: Find items matching any of several categories.

```scheme
(MeetLink
  (Variable "$item")
  (Choice
    (Inheritance (Variable "$item") (Concept "fruit"))
    (Inheritance (Variable "$item") (Concept "vegetable"))
    (Inheritance (Variable "$item") (Concept "grain"))))
```

### Pattern 3: Homogeneity Check with AlwaysLink

**Problem**: Find projects where all team members have required certification.

```scheme
(MeetLink
  (VariableList (Variable "$project") (Variable "$person"))
  (And
    (Member (Variable "$person") (Variable "$project"))
    (Always
      (Evaluation (Predicate "has-certification")
        (List (Variable "$person") (Concept "security-clearance"))))))
```

### Pattern 4: Categorization with GroupLink

**Problem**: Group documents by author, show only prolific authors (3+ docs).

```scheme
(QueryLink
  (VariableList (Variable "$doc") (Variable "$author"))
  (And
    (Evaluation (Predicate "authored-by")
      (List (Variable "$doc") (Variable "$author")))
    (Group
      (Variable "$author")
      (Interval (Number 3) (Number -1))))
  (Variable "$doc"))
```

### Pattern 5: Combining Multiple Advanced Links

**Problem**: Find categories where all items are either red or green.

```scheme
(MeetLink
  (VariableList (Variable "$category") (Variable "$item"))
  (And
    (Member (Variable "$item") (Variable "$category"))
    (Always
      (Choice
        (Evaluation (Predicate "color")
          (List (Variable "$item") (Concept "red")))
        (Evaluation (Predicate "color")
          (List (Variable "$item") (Concept "green")))))))
```

### Pattern 6: Absence in Groups

**Problem**: Find groups that do NOT contain any toxic items.

```scheme
(QueryLink
  (VariableList (Variable "$item") (Variable "$group"))
  (And
    (Member (Variable "$item") (Variable "$group"))
    (Group (Variable "$group"))
    (Always
      (Absent
        (Evaluation (Predicate "is-toxic")
          (Variable "$item")))))
  (Variable "$group"))
```

---

## Testing via MCP Interface

All examples can be tested using the AtomSpace MCP tools:

### 1. Create Test Data

```python
# Via MCP tools
mcp__atomese__makeAtom(atomese="(Concept \"test_item\")")
mcp__atomese__makeAtom(atomese="(Evaluation (Predicate \"color\") (List (Concept \"apple\") (Concept \"red\")))")
```

### 2. Execute Queries

```python
# Execute and get results
result = mcp__atomese__execute(atomese="(MeetLink (Variable \"$x\") (Choice ...))")
print(result)
```

### 3. Verify Results

```python
# Check what's in the AtomSpace
mcp__atomese__reportCounts()
mcp__atomese__getAtoms(type="Concept")
mcp__atomese__getIncoming(atomese="(Concept \"apple\")")
```

---

## Common Pitfalls

### 1. ChoiceLink Nested Incorrectly
**Wrong**: `(MeetLink (Variable "$fruit") (Eval (Pred "x") (Choice (Concept "a") (Concept "b"))))`
**Right**: `(MeetLink (Variable "$fruit") (Choice (Eval (Pred "x") (Concept "a")) (Eval (Pred "x") (Concept "b"))))`

### 2. Confusing AbsentLink with NotLink
- **AbsentLink**: Pattern is not present in AtomSpace
- **NotLink**: Evaluatable expression is false

### 3. AlwaysLink on Single Instance
If there's only one instance, `Always` trivially succeeds. Ensure your pattern can match multiple instances for meaningful results.

### 4. GroupLink Without Present
Using `Group` without `Present` may not work as expected. Always wrap the pattern to be grouped in `Present`.

### 5. Interval Bounds Confusion
- Lower bound is inclusive
- Upper bound of -1 means "no upper limit"
- Constraints apply before rewrite, not after

### 6. Using Deprecated GetLink/BindLink
- Use `MeetLink` instead of `GetLink`
- Use `QueryLink` (with 3 arguments!) instead of `BindLink`

---

## Summary Reference

| Link Type | Purpose | Key Behavior | Use When |
|-----------|---------|--------------|----------|
| **AbsentLink** | Pattern negation | True when pattern NOT in AtomSpace | Excluding results, checking absence |
| **ChoiceLink** | Alternative patterns | Matches ANY of several patterns | Multi-criteria search, fallbacks |
| **AlwaysLink** | Universal quantification | ALL instances must satisfy condition | Homogeneity, constraint validation |
| **GroupLink** | Result partitioning | Groups results by shared variables | Categorization, clustering, aggregation |

---

## Further Reading

- AtomSpace Pattern Matcher Documentation: https://wiki.opencog.org/w/Pattern_matcher
- Query Language Overview: https://wiki.opencog.org/w/QueryLink
- Examples Directory: `/home/opencog/bb/oc/atomspace/examples/pattern-matcher/`
- Individual example files:
  - `absent.scm` - AbsentLink demonstrations
  - `always.scm` - AlwaysLink demonstrations
  - `choice.scm` - ChoiceLink demonstrations
  - `group-by.scm` - GroupLink demonstrations

---

*Document created: 2025-10-03*
*Tested against: AtomSpace 5.0.5 via MCP interface*
*All examples verified with live cogserver instance*

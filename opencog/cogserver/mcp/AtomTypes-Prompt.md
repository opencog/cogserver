# Atom Types Reference - Essential Categories

**Purpose:** Comprehensive reference for Atom types critical for semantic extraction and stream processing

**Status:** Priority categories complete. Full reference in progress.

**Wiki:** https://wiki.opencog.org/w/Atom_types (detailed docs for each type)

---

## Category: Flows - Data Stream Processing

**Purpose:** Process streams of Values through transformation pipelines

### FilterLink
Filters and transforms streams of data, analogous to `filter-map` in functional programming. Takes a pattern or RuleLink and applies it to each element in a stream (from LinkValue, SetLink, ListLink, QueueValue). Can extract values by pattern matching (like "un-beta-reduction") or transform them via rewrite rules. Unlike QueryLink which searches the entire AtomSpace, FilterLink only operates on the provided collection. Type-checking is automatic - mismatched types are silently discarded. Returns same collection type as input (SetLink → SetLink, LinkValue → LinkValue). Essential for pipeline processing. See `/pattern-matcher/examples/filter.scm`.

### ValueOfLink
Retrieves a Value stored at a specific key on an Atom. When executed via `cog-execute!`, fetches the Value and if that Value is itself executable, executes it and returns the result. This dual nature makes it perfect for stream sources: store executable Atoms (like filters or parsers) on Anchors, then ValueOfLink executes them on demand. Not for simple value retrieval - use `cog-value` in Scheme for that. Works with Predicate keys by convention. Returns the Value or the result of executing it. Core component of pipeline architecture.

### SetValueLink
Attaches a Value to an Atom at a specified key, or updates existing Value. Executes as a side-effect operation: takes three arguments (atom, key, value), sets the key-value pair, and returns the value that was set. If the third argument is executable, executes it first before storing. This allows complex formulas: `(SetValue atom key (Plus ...))` computes then stores. Mutable despite Atom immutability - Values can change. Thread-safe for atomic operations. Returns the Value for chaining. Essential for writing pipeline results.

### IncrementValueLink
Atomically increments a numeric Value, providing thread-safe counting and accumulation. Takes an Atom, key, and optional increment amount (defaults to 1). If no Value exists at key, creates FloatValue starting at increment amount. Vector Values are incremented component-wise. Thread-safe via internal locking, allowing concurrent updates without corruption. Commonly used for statistics, event counting, and parallel processing. Returns the new Value after increment. Much faster than read-modify-write in application code.

### DontExecLink
Prevents immediate execution of an executable Atom, storing it for later evaluation. Used when you want to store a "recipe" rather than execute it now. Critical for pipeline setup: `(SetValue anchor key (DontExec (Filter ...)))` stores the Filter without running it. Later, `(ValueOf anchor key)` retrieves and executes it. Without DontExec, executable Atoms would run during SetValue. Also called "quoting for execution" - like QuoteLink but for executable Atoms. Returns the wrapped Atom unchanged.

### PromiseLink
Declares that a Value has a specific type when executed, enabling type-driven stream processing. Format: `(Promise (Type 'TypeName) source)`. Most commonly used with FlatStream to unbundle batched results: `(Promise (Type 'FlatStream) parse-source)` takes a stream producing LinkValues and doles out one element at a time. Lazy evaluation - doesn't execute until consumed. Type argument guides downstream processing. Also works with FutureStream for async computation. Essential for handling parsers that return multiple results per input.

### AnchorNode
Named reference point for storing stream sources and pipeline stages. By convention, uses Predicates as keys: `(SetValue (Anchor "pipeline") (Predicate "stage1") source)`. Enables pipeline organization: each stage stores its output on an Anchor, next stage reads via ValueOfLink. Not special - just a Node type, but conventionally used for this purpose. Helps debugging by naming pipeline components. Multiple pipelines can coexist using different Anchor names. Cleaner than threading stream objects through code.

---

## Category: Streams - AtomSpace to Data Conversion

**Purpose:** Extract data from AtomSpace and convert to stream format

### StringOfLink
Converts between Node types by extracting/using the node name as a string. Format: `(StringOf (Type 'TargetType) source-atom)`. Takes a Node, extracts its name, creates new Node of target type with same name. Example: `(StringOf (Type 'ConceptNode) (Word "cat"))` → `(Concept "cat")`. Executable - use with cog-execute!. Inverse operation also works: extracts name from any Node. Essential for Link Grammar where you extract Word nodes but need Concept nodes for knowledge graphs. Also works in reverse to create typed nodes from strings. Pure type conversion - no string manipulation.

### NumberOfLink
Converts FloatValue to NumberNode by extracting the first component of the vector. FloatValue `[3.14, 2.71]` becomes `(Number "3.14")`. Only uses first element, discards rest. Commonly used after arithmetic operations that return FloatValues when you need a Node in the AtomSpace. Executable via cog-execute!. Counterpart to numeric operations which often return Values. Returns a Node, not a Value.

### CollectionOfLink
Converts between Atoms and Values, primarily LinkValue ↔ SetLink/ListLink. Without type argument, defaults to SetLink. Can specify: `(CollectionOf (Type 'ListLink) linkvalue-source)`. Takes a stream of individual Values/Atoms and bundles them into a Link. Used when FilterLink or other stream operations produce LinkValue but you need actual Atoms in AtomSpace. Type-preserving: respects ordered vs unordered. Inverse operation: can convert SetLink to LinkValue for stream processing. Essential bridge between Value-based pipelines and Atom-based queries.

### SizeOfLink
Returns length of a Value or collection as FloatValue `[n]`. Works on: FloatValue → vector length, StringValue → vector length, LinkValue → number of elements, Atoms with outgoing sets → outgoing set size, NumberNode → 1. Returns single-element FloatValue for easy use in arithmetic. Executable. Common use: check how many items in stream before processing. Also works on Nodes (returns `[1]`) and Links (counts outgoing). Fast operation.

### IncomingOfLink
Retrieves the incoming set of an Atom as a LinkValue containing all Links that reference it. Non-executable - meant for use with FilterLink or other stream processors. The incoming set is unordered, so LinkValue order is arbitrary. Returns empty LinkValue if no incoming Links. Essential for graph traversal: find all relationships involving an entity. Example: `(IncomingOf (Concept "cat"))` returns LinkValue of all Links containing that Concept. Use with FilterLink to process each incoming Link.

### TypeOfLink
Returns the type of an Atom as a TypeNode. Example: `(cog-execute! (TypeOf (Concept "foo")))` → `(Type "ConceptNode")`. Executable. Used in meta-programming and dynamic dispatch: examine atom type to choose processing path. Returns the actual runtime type, not base type. Works with all Atoms. Simple but essential for type introspection.

### KeysOfLink
Returns all keys attached to an Atom as a LinkValue. These are the keys in the Atom's key-value database. Non-executable - use with FilterLink to process. Returns LinkValue of Atoms (usually PredicateNodes). Empty LinkValue if no keys. Essential for discovering what data is attached to an Atom. Use case: find all properties of an entity. Order is arbitrary.

### IsKeyLink
Checks if a specific key exists on an Atom, returns TrueLink or FalseLink. Format: `(IsKey atom key-atom)`. Executable. Used in conditional pipelines to check if data exists before processing. More efficient than fetching all keys. Returns boolean Atom, not Value.

---

## Category: Link Grammar - Natural Language Parsing

**Purpose:** Parse natural language text using Link Grammar parser

### LgParseBonds
Modern Link Grammar parser that returns bond structures as LinkValue. Format: `(LgParseBonds (Phrase "text") (LgDict "en") (Number num-linkages))`. Returns nested LinkValue: outer contains one LinkValue per linkage (parse), each containing word-list and bond-list. Bond-list has Edge atoms: `(Edge (Bond "Ss*s") (List (Word "cat") (Word "sat")))`. Number argument requests multiple parses (4 is common). Use with FilterLink + Glob patterns to extract specific bonds. Preferred over LgParseLink (old format). Pure data return - no AtomSpace pollution.

### Phrase (aka PhraseNode)
Input text for Link Grammar parser. Simply wraps a string: `(Phrase "The cat sat")`. Not the parsed result - just the input. Node type created for type safety. Use as first argument to LgParseBonds. UTF-8 text.

### LgDict (aka LgDictNode)
Specifies which Link Grammar dictionary to use. `(LgDict "en")` for English, `(LgDict "ru")` for Russian, etc. Required argument to LgParseBonds. Dictionaries must be installed separately. Dictionary determines what languages can be parsed and what link types are recognized.

### Word (aka WordNode)
Individual word in a Link Grammar parse. Created by parser, not manually. Name is the actual word: `(Word "cat")`. Appears in bond structures. Type-separate from ConceptNode intentionally - distinguishes syntax (Word) from semantics (Concept). Use StringOfLink to convert: `(StringOf (Type 'ConceptNode) (Word "cat"))` → `(Concept "cat")`.

### Bond (aka BondNode)
Link type in Link Grammar parse, like "Ss*s" (subject-verb singular), "Os" (object), "MVp" (verb-modifier past). Created by parser in Edge structures: `(Edge (Bond "Ss*s") (List word1 word2))`. Name is the Link Grammar link type. Hundreds of types, see LG documentation. Different bonds indicate different grammatical relationships. Essential for pattern matching to extract semantic relationships.

### LgParseLink, LgParseMinimal
Old-style parsers that create many Atoms in AtomSpace (deprecated). Use LgParseBonds instead. These create complex Atom structures that clutter AtomSpace. LgParseMinimal omits disjuncts but still verbose. Kept for backwards compatibility only.

### LgParseDisjuncts, LgParseSections
Specialized parsers returning disjuncts or Section atoms instead of bonds. Disjuncts show how words connect. Sections are sheaf-theoretic structures for advanced use. Most users want LgParseBonds. These are for linguistic research or specialized processing.

---

## Category: Pattern Matching - Queries and Variables

**Purpose:** Core pattern matching constructs for querying AtomSpace

### MeetLink
Finds all Atoms matching a pattern, returns results as QueueValue (thread-safe stream). Modern replacement for GetLink (which returned SetLink). Format: `(Meet vardecls pattern)`. Vardecls can be Variable, TypedVariable, or VariableList. Pattern uses variables as placeholders. Returns QueueValue containing all groundings: each element is the variable binding (single Variable) or ListLink (multiple variables). Non-polluting: doesn't create SetLinks in AtomSpace. Execute with cog-execute!. For patterns only - no rewriting. See `/pattern-matcher/examples/` for extensive examples.

### QueryLink
Pattern matching WITH rewriting: finds matches and creates new Atoms based on template. Format: `(Query vardecls pattern rewrite)`. Like MeetLink but adds third argument: rewrite template. For each match, substitutes variables in rewrite template with matched values. Returns QueueValue of rewritten Atoms. Mathematically: MeetLink + PutLink. Use when you want to transform matches, not just find them. Replaces deprecated BindLink (which returned SetLink). Core tool for inference and rule application.

### RuleLink
Pattern-matching rewrite rule for use with FilterLink. Format: `(Rule vardecls pattern rewrite)`. Like LambdaLink but with rewrite clause. FilterLink applies Rule to each stream element: if element matches pattern, returns rewrite; if no match, discards. Used for stream transformation. Not for AtomSpace queries - use QueryLink for that. Essential for pipeline processing: `(Filter (Rule ...) stream)`. Pattern can use Glob for flexible matching.

### GlobNode
Matches zero or more consecutive Atoms in a sequence. Like regex `*` but for Atoms. Format: `(Glob "$varname")`. Must appear in ordered context (ListLink, not SetLink). Example: `(List (Concept "I") (Glob "$middle") (Concept "you"))` matches any list starting with "I" and ending with "you", binding `$middle` to everything between. Critical for LinkValue processing where you don't know how many bonds appear before/after target. Type-constrainable: `(TypedVariable (Glob "$g") (Type 'Word))`. Must match at least one atom (currently - may change).

### VariableNode
Pattern variable for matching single Atoms. Format: `(Variable "$name")`. Dollar-sign convention but not required. In pattern, acts as placeholder: `(Edge (Predicate "rel") (List (Variable "$x") (Concept "target")))` matches any first element. Grounding is the matched Atom. Use TypedVariable to constrain type. Distinguished from regular Nodes by context (pattern matching) not type alone.

### TypedVariableLink
Constrains pattern variable to specific type(s). Format: `(TypedVariable (Variable "$x") (Type 'ConceptNode))`. Variable `$x` will only match ConceptNodes. Type can be base type (matches all subtypes) or use TypeChoice for alternatives. Essential for reducing search space and preventing nonsensical matches. Works with VariableNode and GlobNode. Enables type-safe pattern matching.

### VariableList
Declares multiple variables for pattern. Format: `(VariableList (Variable "$x") (Variable "$y") ...)`. Can mix Variable and TypedVariable. Order doesn't matter for matching but affects result structure: MeetLink returns ListLink in declaration order. Alternative to listing variables separately. Required when pattern has multiple variables.

### SignatureLink
Type pattern for matching Atom structure. Uses TypeNodes instead of Variables. Format: `(Signature (Type 'EdgeLink) (Type 'PredicateNode) (Type 'ListLink))`. Matches any Edge with Predicate and List children. More rigid than Variables - only checks types, doesn't bind values. Used with FilterLink for type-based filtering. Polymorphic via TypeChoice. Different from LinkSignature (which constructs LinkValues).

### LinkSignature (aka LinkSignatureLink)
Constructs LinkValue in rewrite rules. Format: `(LinkSignature (Type 'LinkValue) elem1 elem2 ...)`. Essential for FilterLink rewrites operating on LinkValues. Creates new LinkValue containing specified elements. Can nest: `(LinkSignature (TypeInh 'LinkValue) ...)` with type inheritance. Not a pattern - a constructor. Used in Rule rewrite clauses to build output.

---

## Quick Reference - Common Tasks

**Parse text and extract semantic relationships:**
```scheme
; 1. Parse with Link Grammar
(LgParseBonds (Phrase "cat sat") (LgDict "en") (Number 1))

; 2. Extract specific bond with Filter + Glob
(Filter
  (Rule
    (VariableList ...)
    (LinkSignature ... (Glob "$before") (Edge (Bond "Ss*s") ...) (Glob "$after")))
    <rewrite>)
  <parse-source>)

; 3. Convert Word → Concept
(StringOf (Type 'ConceptNode) (Word "cat"))
```

**Process a data stream:**
```scheme
; Setup pipeline
(SetValue (Anchor "pipeline") (Predicate "source")
  (DontExec <source-expression>))

; Filter/transform
(Filter
  (Rule <vardecls> <pattern> <rewrite>)
  (ValueOf (Anchor "pipeline") (Predicate "source")))
```

**Query AtomSpace:**
```scheme
; Find matches only
(Meet
  (TypedVariable (Variable "$x") (Type 'ConceptNode))
  (Edge (Predicate "rel") (List (Variable "$x") (Concept "target"))))

; Find and transform
(Query <vardecls> <pattern> <rewrite-template>)
```

---

## Examples to Study

**Essential (read in order):**
1. `/atomspace/examples/atomspace/flows.scm` - ValueOfLink, SetValueLink
2. `/atomspace/examples/atomspace/stream.scm` - Stream processing
3. `/pattern-matcher/examples/filter.scm` - FilterLink with RuleLink
4. `/pattern-matcher/examples/glob.scm` - Glob patterns
5. `/sensory/examples/parse-pipeline.scm` - Complete LG pipeline

**Your working examples:**
- `/diary/lg_learning/complete_svo_extraction.scm` - Extraction patterns
- `/diary/lg_learning/FILTER_LINKVALUE_GUIDE.md` - Your documentation

---

## Notes

- All executable Atoms use `cog-execute!` in Scheme
- MCP execute tool works on same Atoms
- Stream types: QueueValue (FIFO), LinkValue (vector), SetLink (unordered)
- FlatStream unbundles batched results
- Glob matches 1+, not 0+ (currently)
- Bond types from LG dict, see LG documentation
- Type hierarchy: use getSuperTypes/getSubTypes MCP tools

**Last updated:** 2025-10-28
**Status:** Priority categories complete - covers semantic extraction essentials

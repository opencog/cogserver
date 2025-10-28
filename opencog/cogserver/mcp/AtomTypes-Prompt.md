# Atom Types Reference

**Purpose:** Comprehensive reference for 170+ Atom types organized by functional category

**Status:** Complete - covers all major categories with 3-10 sentence descriptions per type

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

### LgParseDisjuncts, LgParseSections
Specialized parsers returning disjuncts or Section atoms instead of bonds. Disjuncts show how words connect. Sections are sheaf-theoretic structures for advanced use. Most users want LgParseBonds. These are for linguistic research or specialized processing.

---

## Category: Pattern Matching - Queries and Variables

**Purpose:** Core pattern matching constructs for querying AtomSpace

### MeetLink
Finds all Atoms matching a pattern, returns results as QueueValue (thread-safe stream). Format: `(Meet vardecls pattern)`. Vardecls can be Variable, TypedVariable, or VariableList. Pattern uses variables as placeholders. Returns QueueValue containing all groundings: each element is the variable binding (single Variable) or ListLink (multiple variables). Non-polluting: doesn't create Atoms in AtomSpace for results. Execute with cog-execute!. For patterns only - no rewriting. See `/pattern-matcher/examples/` for extensive examples.

### QueryLink
Pattern matching WITH rewriting: finds matches and creates new Atoms based on template. Format: `(Query vardecls pattern rewrite)`. Like MeetLink but adds third argument: rewrite template. For each match, substitutes variables in rewrite template with matched values. Returns QueueValue of rewritten Atoms. Mathematically: MeetLink + PutLink. Use when you want to transform matches, not just find them. Core tool for inference and rule application.

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

## Category: Scoping & Lambda Calculus

**Purpose:** Variable binding, lambda expressions, beta reduction

### LambdaLink
Lambda abstraction for creating anonymous functions. Format: `(Lambda vardecls body)`. Vardecls specify parameters, body is expression using those variables. When executed with PutLink, performs beta reduction: substitutes arguments for variables. Used to define DefinedProcedureNode and DefinedSchemaNode bodies. Unlike ScopeLink (change of variable only), LambdaLink allows free beta-reduction with any values. Example: `(Lambda (Variable "$x") (Plus (Variable "$x") (Number 1)))` is increment function. Execute via PutLink to apply.

### PutLink
Beta reduction: substitutes values for variables in a lambda expression or pattern. Format: `(Put lambda-or-pattern (List arg1 arg2 ...))` or `(Put vardecls body (List args))`. Creates new Atom/Value with variables replaced by arguments. Core of function application in Atomese. QueryLink = MeetLink + PutLink conceptually. Can explicitly specify variables as ScopeLink. Returns the result of substitution. Example: `(Put (Lambda (Variable "$x") (Plus (Variable "$x") (Number 1))) (List (Number 5)))` → `(Number 6)`.

### ScopeLink
Base class for links with scoped variables (LambdaLink, QueryLink, MeetLink, etc.). Provides variable scoping infrastructure. Not typically used directly - use subclasses. Defines variable scope boundaries. Only allows beta-reductions equivalent to change-of-variable (alpha conversion), unlike LambdaLink's free reduction. Ensures variable names don't clash. Foundation for all variable-using Atoms.

### JoinLink
Pattern matching for partial graphs: finds containing structures when given subgraphs. Dual to MeetLink. While MeetLink finds subgraphs matching a pattern, JoinLink finds "super-graphs" containing known elements. Format: `(Join vardecls pattern)`. Searches for unknown containers of known subgraphs. Named for lattice theory "join" operation (MeetLink is "meet"). Less commonly used than MeetLink but powerful for structural queries. Returns QueueValue of matching containers.

### DualLink
Inverse pattern matching: given a ground term (answer), finds patterns (questions) that would match it. Used for pattern recognition and rule engine construction. Finds all queries/rules that this data satisfies. Essential for SRAI/chatbot systems: match input to response patterns. Also useful for finding applicable rules in forward chaining. Given concrete data, returns patterns with variables that would match. Enables meta-reasoning: "what rules apply here?"

### FreeLink
Marks variables as free (not to be bound) in a pattern. Normally all variables in scope are bound; FreeLink prevents binding specific ones. Used when you want some variables to remain as variables in result. Format: `(Free (Variable "$x") body)`. Variable `$x` stays as `$x` instead of being grounded. Rare use case but essential when needed.

### QuoteLink, UnquoteLink
Prevents pattern matching/execution within quoted region. QuoteLink treats contents literally - variables aren't variables, executable Links aren't executed. UnquoteLink escapes back to normal interpretation within quoted region. Similar to Lisp quote/unquote. Used when you want to match the structure of a pattern itself, not use it as a pattern. LocalQuoteLink is variant for local quoting.

---

## Category: Arithmetic & Numeric Functions

**Purpose:** Symbolic and computational arithmetic

### PlusLink
Addition operation, both symbolic and computational. Format: `(Plus arg1 arg2 ...)`. Works on NumberNodes, FloatValues, returns appropriate type. Performs term reduction: `0 + x → x`, `2 + 3 → 5`, `x + x → 2*x`. Vector addition component-wise. Symbolic: represents formula in AtomSpace for learning/reasoning. Computational: actually computes when executed. Built on FoldLink for associative reduction. Fast but not CPU-native speed (100x slower than raw CPU).

### TimesLink
Multiplication with symbolic manipulation and computation. Format: `(Times arg1 arg2 ...)`. Reductions: `1 * x → x`, `0 * x → 0`, `2 * 3 → 6`. Component-wise for vectors. Can multiply matrices via vector operations. Accumulate sums components; Times multiplies them. Also built on FoldLink. Used extensively in formula representation for PLN, learning systems.

### MinusLink
Subtraction: `(Minus arg1 arg2)`. Binary operation. Performs reductions: `x - 0 → x`, `x - x → 0`, `5 - 3 → 2`. Vector subtraction component-wise. Less commonly used than Plus (can use Plus with negative numbers).

### DivideLink
Division: `(Divide numerator denominator)`. Binary operation. Reductions: `x / 1 → x`, `0 / x → 0`, `6 / 2 → 3`. Component-wise for vectors. Beware division by zero - may throw or return NaN depending on context.

### AccumulateLink
Sums all components of a vector FloatValue. `FloatValue [1, 2, 3]` → `FloatValue [6]`. Reduces vector to single number. Essential for vector processing pipelines. Different from PlusLink which adds multiple vectors component-wise. Common pattern: arithmetic on vectors → AccumulateLink → NumberOfLink → Node.

### FoldLink
Generic associative reduction operation over lists. PlusLink and TimesLink built on top of this. Implements comp-sci fold/reduce concept. Takes binary operation and list, applies operation iteratively. Most users use PlusLink/TimesLink rather than FoldLink directly. Foundation for iterated operations.

### GreaterThanLink, LessThanLink, EqualLink
Comparison operations returning TrueLink or FalseLink. Virtual links - don't store all possible comparisons, compute on demand. Example: `(GreaterThan (Number 5) (Number 3))` → TrueLink. Work with NumberNodes and FloatValues. Essential for conditional logic. Used in pattern matching as evaluatable conditions. GreaterThan and others allow reasoning about order without computing all combinations.

### ExpLink, LogLink, SineLink, CosineLink, TanLink, PowLink
Mathematical functions for scientific computation. All executable. ExpLink is e^x, LogLink is natural log, trig functions self-explanatory. PowLink is exponentiation: `(Pow base exponent)`. Work on NumberNodes and FloatValues. Used in formulas for PLN, statistics, signal processing. Combined with arithmetic for complex expressions.

### MinLink, MaxLink
Return minimum/maximum of arguments. `(Min arg1 arg2 ...)` returns smallest. Works on numbers and vectors (component-wise). Used in optimization, bounds checking. Executable.

### FloorLink, HeavisideLink, ImpulseLink
Specialized math functions. FloorLink rounds down. HeavisideLink is step function (0 below threshold, 1 above). ImpulseLink is delta function. Used in signal processing and piecewise functions.

### RandomNumberLink
Generates random number when executed. Returns FloatValue. Non-deterministic - each execution gives different result. Seed not controllable via Atomese (uses system random). Used for probabilistic algorithms.

---

## Category: Boolean Logic & Conditions

**Purpose:** Logical operations and conditional execution

### AndLink, OrLink, NotLink
Classical boolean logic on TrueLink/FalseLink. AndLink: all arguments must be True. OrLink: any argument can be True. NotLink: inverts single argument. Executable - evaluate boolean sub-expressions. Used in pattern matching conditions and conditional execution. Different from SequentialAndLink (which sequences actions).

### TrueLink, FalseLink
Boolean constants. No arguments, just represent True/False. Returned by comparisons and predicates. Used as success/failure indicators. Execute to themselves.

### SequentialAndLink, SequentialOrLink
Execute arguments in order, short-circuit on failure/success. SequentialAnd stops at first failure. SequentialOr stops at first success. Unlike AndLink/OrLink which are declarative, these are imperative control flow. Used for sequencing actions where order matters. Returns last executed value.

### ChoiceLink
Try alternatives in order until one succeeds. Format: `(Choice option1 option2 ...)`. Executes options sequentially, returns first non-failing result. Used for fallback logic. Pattern matching: try multiple patterns. Different from OrLink which evaluates all.

### PresentLink
Tests if pattern exists in AtomSpace, returns TrueLink/FalseLink. Like SatisfactionLink but returns boolean. Used in conditional logic: "if this pattern exists, then...". Evaluatable in queries. Finds at least one grounding of pattern. Non-polluting check for existence.

### AbsentLink
Negation: pattern must NOT exist. Returns TrueLink if pattern not found. Implements intuitionistic logic - represents "unknown" not "known false". Variables in Absent leak to outer scope. Essential for negative constraints: "find X that is NOT Y". Used with And to combine positive and negative conditions. See `/pattern-matcher/examples/absent.scm`.

### AlwaysLink
Universal quantifier: pattern must hold for ALL instances. "For-all" semantics. Different from pattern matching's default "there-exists". Tests that condition holds universally. Returns TrueLink if all instances satisfy, FalseLink otherwise. Used for global constraints.

### GroupLink
Groups results by shared variable values, like SQL GROUP BY. Returns nested structure: outer level groups, inner level group members. Used when you want to cluster results. Example: group people by city. Less common than other pattern atoms but powerful for aggregation.

---

## Category: External Systems - Callbacks & Integration

**Purpose:** Call external code (Python, Scheme, C++) from Atomese

### GroundedSchemaNode
Calls external code when executed. Format: `(GroundedSchema "lang:function")`. Language prefixes: `py:` for Python, `scm:` for Scheme, `lib:` for C++ shared libraries. Function must be in PYTHONPATH/load-path. Used with ExecutionOutputLink to pass arguments. Returns Value. Example: `(GroundedSchema "py:my_function")` calls Python function `my_function`. Essential bridge to external processing, sensors, actuators. Allows Atomese to control external systems.

### GroundedPredicateNode
External predicate returning boolean (TrueLink/FalseLink). Format: `(GroundedPredicate "lang:function")`. Used in pattern matching to check external conditions. Example: sensor reading above threshold. Function receives arguments, returns boolean. Enables reactive behavior: patterns trigger only when external condition holds. Used in robotics, game AI, chatbots for environmental awareness.

### DefinedSchemaNode
Named Atomese function defined via DefineLink. Pure Atomese (no external code). Format: Define a name, then reference it. Example: `(Define (DefinedSchema "increment") (Lambda ...))`. Reusable functions in Atomese. Cleaner than copying lambdas everywhere. Can be recursive. Preferred over Grounded when logic can be expressed in Atomese.

### DefinedProcedureNode
Like DefinedSchema but for procedures (side effects OK). Named via DefineLink. Returns general Values, not just specific types. Used for complex Atomese procedures that modify state or produce varied outputs. Distinction from Schema is historical/semantic - both work similarly.

### DefinedPredicateNode
Named boolean predicate in pure Atomese. Defined via DefineLink with boolean-returning body. Used in pattern matching like GroundedPredicate but implemented in Atomese. Reusable logical conditions.

### ExecutionOutputLink
Executes Schema/Procedure with arguments. Format: `(ExecutionOutput schema (List arg1 arg2 ...))`. Schema can be Grounded* or Defined*. Passes arguments to function/procedure, returns result. Execute with cog-execute!. Essential for actually calling external code or defined functions. Arguments available to external function as list of Atoms.

### SchemaNode, ProcedureNode
Base classes. SchemaNode for functions, ProcedureNode for procedures. Not usually instantiated directly - use Grounded* or Defined* subtypes. Provide type hierarchy for callable entities.

---

## Category: Storage & Persistence

**Purpose:** Save/load AtomSpace to disk, network, databases

### StorageNode
Base class for all persistence backends. Provides store/fetch/load operations. Not used directly - use specific subtypes. Enables AtomSpace persistence and distribution. Load brings Atoms from storage into RAM. Store writes Atoms from RAM to storage. Fetch retrieves specific Atoms. All operations preserve uniqueness.

### RocksStorageNode
RocksDB backend - recommended for local persistence. Format: `(RocksStorage "rocks:///path/to/db")`. Fast, embedded database. Stores Atoms and Values. Good for single-machine persistence. No network overhead. Handles millions of Atoms efficiently. Use for checkpointing, long-term storage.

### FileStorageNode
Plain text file storage in s-expression format. Human-readable. Format: `(FileStorage "file:///path/to/file.scm")`. Slow but debuggable. Good for small AtomSpaces, configuration, sharing. Not for large datasets.

### MonoStorageNode
Single-file compact format. Fast loading. Format: `(MonoStorage "mono:///path/to/file")`. Optimized for bulk load/store. Less flexible than Rocks but faster for checkpoint/restore.

### CogStorageNode, CogSimpleStorageNode
Network client to remote CogServer. Format: `(CogStorage "cog://hostname:port")`. Enables distributed AtomSpace. Multiple clients can connect to same server. CogSimple is lightweight version. Used for multi-machine setups, cloud deployments. Atoms fetched on demand over network.

### ProxyNode
Composite/proxy pattern for multiple backends. Base class for various proxy types. Enables mirroring, load balancing, caching strategies. Wraps multiple StorageNodes.

### ReadThruProxyNode, WriteThruProxyNode, ReadWriteProxyNode
Caching proxies. ReadThru: read from storage, cache in RAM. WriteTh

ru: write immediately to storage. ReadWrite: both. Used to optimize storage access patterns. Reduces network/disk latency.

### WriteBufferProxyNode
Batches writes to reduce I/O. Accumulates changes, flushes periodically. Improves performance when many small updates. Trade-off: delayed persistence for throughput.

### SequentialReadProxyNode
Reads from multiple storages in sequence until found. Enables fallback chains. Try local cache, then remote server, then archive.

### NullProxyNode, CachingProxyNode, DynamicDataProxyNode
Specialized proxies. Null discards data (testing). Caching adds caching layer. DynamicData for generated/computed data.

### FetchValueOfLink, StoreValueOfLink
Fetch/store specific Values from storage. Like ValueOfLink but for persistence. Fetch loads from remote/disk. Store saves immediately. Granular control over what gets persisted.

---

## Category: Execution Control & State

**Purpose:** Control flow, state management, threading

### CondLink
Conditional execution (if-then-else). Format: `(Cond condition then-clause else-clause)`. Evaluates condition (boolean or pattern), executes appropriate branch. Returns result of executed branch. Used for branching logic. Condition can be comparison, PresentLink, GroundedPredicate, etc.

### StateLink
Unique state: ensures only one value for a key. Format: `(State key value)`. When new State created with same key, old one removed. Thread-safe. Used for singleton state like "current mode", "active user". Differs from SetValue (which allows multiple values per key in key-value database). StateLink is in the graph, SetValue is attached data.

### DefineLink
Names an Atom for reuse. Format: `(Define name-atom body)`. Name is usually DefinedPredicate/Schema/Procedure. Body is the definition. Enables named functions, predicates, patterns. Once defined, use name to reference body. Can be redefined (replaces previous).

### DeleteLink
Removes Atom from AtomSpace. Format: `(Delete atom)` or `(Delete atom recursive-flag)`. Immediate removal. Recursive deletes all incoming Links too. Use carefully - breaks references. Returns deleted Atom.

### UniqueLink
Creates unique instance even if duplicate structure. Overrides normal uniqueness constraint. Rarely needed. Used when you need multiple "identical" Atoms distinguished by instance.

### SequenceLink
Executes arguments in order, returns last result. Format: `(Sequence step1 step2 ...)`. Pure sequencing - all steps execute regardless. Different from SequentialAnd (short-circuit). Used for ordered side effects.

### ParallelLink
Creates threads, executes arguments in parallel. Format: `(Parallel task1 task2 ...)`. Each argument runs in new thread. Returns ListLink of results (order matches argument order). Blocks until all complete. Thread-safe - AtomSpace handles locking. Use for CPU-bound parallel work.

### ExecuteThreadedLink
Older parallel execution. ParallelLink preferred. Similar functionality.

### SleepLink
Delays execution. Format: `(Sleep (Number seconds))`. Blocks for specified time. Returns after delay. Used for timing, rate limiting. Seconds can be fractional.

### RandomChoiceLink
Randomly selects one argument to execute. Format: `(RandomChoice option1 option2 ...)`. Equal probability (currently). Returns result of chosen option. Non-deterministic. Used for stochastic behavior.

### GrantLink
Thread-safe state update with locking. More sophisticated than StateLink. Prevents race conditions in complex updates.

---

##  Category: Type System

**Purpose:** Runtime type system, type constructors, signatures

### TypeNode
References an Atom type by name. Format: `(Type 'ConceptNode')` or `(Type "ConceptNode")`. Used in TypedVariable, type patterns, type checking. Not the type itself - a reference to it. Enables meta-programming on types. Can get supertypes/subtypes dynamically.

### TypeChoice
Union type: accepts any of several types. Format: `(TypeChoice (Type 'ConceptNode) (Type 'PredicateNode'))`. Used in TypedVariable for polymorphism. Variable can match any listed type. Enables flexible patterns.

### DefinedTypeNode
User-defined type name. Created via DefineLink. Allows custom type aliases. Less common than TypeNode.

### SignatureLink
Type signature for pattern matching (already covered in Pattern section - here for completeness). Declares expected structure using types.

### ArrowLink
Function type: input types → output type. Format: `(Arrow input-types output-type)`. Describes function signatures. Used in type checking and inference. Enables typed lambda calculus.

### TypeIntersectionLink
Intersection of types: must satisfy all. Less common than TypeChoice. Used for multi-constraint type specifications.

### TypeInheritanceLink (TypeInhNode, TypeCoInhNode)
Type inheritance relationships. Rarely used directly - type hierarchy built into C++ classes. TypeInhNode checks if subtype, TypeCoInhNode for covariance. Advanced type theory features.

### TypedAtomLink
Associates type signature with an Atom. Meta-type information. Used in type inference systems.

### IntervalLink
Numeric range type. Specifies min/max bounds. Used for bounded numeric types. Type constructor for intervals.

---

## Category: NLP - Natural Language Processing

**Purpose:** Represent linguistic structures and parse results

### WordNode
Represents a word in text. Name is the word itself: `(Word "cat")`. Used in NLP processing to track vocabulary. Different from Word (Link Grammar) which is parser output. WordNode is general-purpose word representation. Used in language learning, semantic graphs. Can have linguistic properties attached via Values.

### SentenceNode
Anchor for sentence parses and linguistic analysis. One SentenceNode per sentence. ParseNodes link to SentenceNode via ParseLink to show different parses of same sentence. Name typically identifies sentence uniquely. Used to group all parse interpretations together.

### ParseNode
Represents a specific parse of a sentence. Links to SentenceNode via ParseLink. Multiple ParseNodes can exist for one SentenceNode (different interpretations). Contains or references parse tree structure. Used in ambiguity resolution, parse ranking.

### DocumentNode
Represents a document or text collection. Can contain multiple sentences. Used to group related content. Name identifies document. Enables document-level analysis and processing.

### WordInstanceNode
Unique instance of word occurrence in a parse. Same word appearing twice gets two WordInstanceNodes with unique names (e.g., "cat@123", "cat@456"). Needed because pattern matching requires unique atoms for each occurrence. Links to WordNode to show which word it instantiates.

### WordInstanceLink
Connects WordInstanceNode to its WordNode. Shows this instance is of that word type. Used to resolve instances back to word types.

### ReferenceLink
Links words/concepts to their referents or definitions. Used in coreference resolution, entity linking. Shows what a word refers to in context.

### SentenceLink
Associates sentence structure. May link parse to sentence. Various NLP relationships at sentence level.

### SequenceLink, WordSequenceLink, SentenceSequenceLink, DocumentSequenceLink
Ordered sequences of linguistic elements. WordSequence for word order. SentenceSequence for sentence order in paragraph. DocumentSequence for document order. Preserve ordering critical for language processing. Used in n-grams, context windows, text generation.

### WordClassNode
Class or category of words (e.g., "nouns", "animals"). Used for word type classification. Groups words with similar properties. Enables class-based operations.

---

## Category: Sensory I/O - External System Interaction

**Purpose:** Interface with files, terminals, network, external data (requires sensory module)

### SensoryNode
Base class for sensory-motor objects that interact with external systems. Subclass of ObjectNode. Implements message-passing interface with open/close/read/write messages. Foundation for all I/O atom types. Not instantiated directly - use specialized subtypes.

### TextFileNode, TextStreamNode
File I/O for text data. TextFile reads from files on disk. Format: `(TextFile "file:///path/to/file.txt")`. Supports streaming modes: read entire file, tail mode (like `tail -f`), or line-by-line. Open with `*-open-*` message, read with `*-read-*` or `*-stream-*`. TextStream is more general stream abstraction. Returns StringValue or stream of lines. Essential for file-based pipelines.

### FileSysNode
Filesystem navigator and monitor. Can watch directories with inotify for file changes. List directory contents. Monitor file creation/deletion. Format: `(FileSys "file:///path/to/dir")`. Streams filesystem events as they occur. Used for reactive systems that respond to file changes. Returns stream of ItemNodes (filenames).

### TerminalNode
Interactive terminal I/O via xterm or similar. Bidirectional: read from terminal, write to terminal. Format: `(Terminal "xterm:display")`. Opens xterm window, captures input, displays output. Used for interactive agents, debugging, demos. Reads return StringValues, writes display text. See `xterm-io.scm` example.

### IRChatNode (or IrcNode)
IRC chat client interface. Connect to IRC servers, join channels, send/receive messages. Format: `(IRC "irc://server:port/channel")`. Streams incoming messages, sends outgoing. Used for chatbots, monitoring, communication agents. Messages as StringValues with metadata. See `irc-api.scm`, `irc-echo-bot.scm` examples.

### StreamNode
Base class for streaming data sources. Provides stream interface. Subclassed by specific stream types. Not used directly.

### ObjectNode
Base class for message-passing objects representing external entities or systems. Every ObjectNode has key-value database where specific keys are interpreted as messages (methods). Getting/setting message keys invokes behavior. Convention: `*-open-*`, `*-close-*`, `*-read-*`, `*-write-*` as standard messages. Subclass to create new external interfaces. Essential pattern for extensibility.

---

## Category: Graph Representation - Knowledge Graphs

**Purpose:** Basic graph database structures for knowledge

### ConceptNode
Represents a concept or entity in knowledge base. Name is concept identifier: `(Concept "cat")`, `(Concept "animal")`. Most common Node type. Used in semantic networks, ontologies, knowledge graphs. Can have properties via Values, relationships via Links. Foundation of knowledge representation.

### PredicateNode
Represents a relation, property, or predicate. Name is predicate identifier: `(Predicate "has-color")`, `(Predicate "larger-than")`. Used in EdgeLink to label relationships. Also used as keys in key-value databases by convention. Distinguishes relationships from entities.

### ItemNode
Generic item or vertex in graph. More neutral than Concept. Used when item isn't really a concept - just a graph node. Filenames, identifiers, arbitrary vertices. Less semantic weight than Concept.

### TagNode
Tagging and categorization. Lightweight labels. Name is the tag. Used to mark or categorize other Atoms. Can create tag clouds, folksonomy structures.

### TagLink
Associates tags with atoms. Links TagNode to tagged Atom. Enables tag-based retrieval and organization.

### EdgeLink
Labeled directed edge in knowledge graph. **Canonical form:** `(Edge (Predicate "relation") (List source target))`. Predicate names the relationship. List contains endpoint vertices (usually ItemNode or ConceptNode). Can have more than 2 endpoints for n-ary relations. Modern replacement for EvaluationLink. Uses less RAM/CPU. Preferred for graph databases.

### ListLink
Ordered list of Atoms. Format: `(List atom1 atom2 ...)`. Order matters. Used everywhere: function arguments, sequences, compound structures. Fundamental building block. Immutable like all Links.

### SetLink
Unordered set of Atoms. Format: `(Set atom1 atom2 ...)`. Order doesn't matter: `(Set A B)` equals `(Set B A)`. Used for collections where order irrelevant. Note: for stream processing results, QueueValue preferred over SetLink.

### MemberLink
Set membership: element is member of set. Format: `(Member element set)`. Used in knowledge graphs for category membership. Different from contains-in-list.

### SubsetLink
Subset relation between sets. Format: `(Subset smaller-set larger-set)`. Used in taxonomies, hierarchies. All members of first are members of second.

### InheritanceLink, SimilarityLink
Inheritance and similarity relationships. InheritanceLink: `(Inheritance subtype supertype)` for is-a relationships. SimilarityLink: `(Similarity thing1 thing2)` for similarity. Used in ontologies, semantic networks. Enable reasoning about categories and analogies.

### ContextLink
Associates context with a statement or atom. Enables context-dependent truth, temporal logic. Less commonly used but powerful for modal logic.

---

## Category: Vector Operations - Matrix/GPU Processing

**Purpose:** Efficient vector and matrix operations for ML pipelines

### ElementOfLink
Extracts element from vector. Format: `(ElementOf (Number index) value-with-vector)`. Index 0 is first element. Works on FloatValue, StringValue, LinkValue. Returns single element as appropriate Value type. Used to unpack vectors for scalar operations. Example: `(ElementOf (Number 0) (ValueOf atom key))` gets first component.

### DecimateLink
Downsamples vector by factor. Keeps every Nth element. Format: `(Decimate (Number N) vector-value)`. Used in signal processing, data reduction. Returns smaller FloatValue.

### Column, FloatColumn, LinkColumn, SexprColumn, TransposeColumn
Column types for matrix operations and GPU processing. FloatColumn holds column of floats. LinkColumn holds column of Atoms. Used to pack data for vectorized operations. TransposeColumn transposes matrix. Enables batch processing, SIMD operations. Bridges to GPU computation. See `vector-column.scm` example.

### BoolOpLink, BoolAndLink, BoolOrLink, BoolNotLink
Boolean operations on boolean vectors (BoolValue). Component-wise AND, OR, NOT on bit vectors. Different from AndLink (scalar boolean logic). Used for bitwise operations, masks, filters on vector data.

---

## Category: Sheaves - Linguistic & Chemical Structures

**Purpose:** Partially assembled structures, connectors (chemistry, linguistics)

### Section
Partially assembled structure in sheaf theory. Represents incomplete molecule, sentence fragment, or pattern. Can be combined with other Sections via connectors. Foundation for compositional assembly. Used in chemistry for molecule building, in linguistics for grammar.

### Connector
Connection point for assembly. Has directionality and type. Sections expose Connectors showing how they can combine. Mating Connectors creates assemblies. Linguistic: verb needs subject (Connector). Chemical: bond site on molecule.

### ConnectorSeq, ConnectorSet, ConnectorChoice
Collections of Connectors. ConnectorSeq is ordered sequence. ConnectorSet is unordered. ConnectorChoice offers alternatives. Used in Section definitions to specify multiple connection possibilities.

### ConnectorDir
Directional connector - has source/sink polarity. Used in directed assembly. Ensures proper orientation.

### SexNode (Section Node?)
Node type related to sections or partial structures. Name unclear - may be "Section Node" or different semantic.

### ShapeLink
Specifies geometric or topological shape. Used in spatial reasoning, molecular geometry. Associates shape properties with structures.

### CrossSection
Intersection or cross-section of sections. Sheaf theory operation. Finds common structure between partial assemblies.

### LgConnNode, LgConnMultiNode, LgConnDirNode, LgConnector, LgSeq, LgAnd, LgOr, LgWordCset, LgDisjunct, LgLinkNode, LgLinkInstanceNode, LgLinkInstanceLink
Link Grammar internal types for dictionary representation and disjunct structures. LgConn* types represent connectors in LG formalism. LgDisjunct is disjunctive expression of connector requirements. LgSeq/And/Or are logical combinations. These are advanced LG internals - most users use LgParseBonds output, not these directly. For LG dictionary manipulation and linguistic research.

### LgHaveDictEntry, LgDictEntry
Link Grammar dictionary entries. Check if word has entry, get entry details. Used for dictionary queries and validation.

---

## Category: Utilities & Base Types

**Purpose:** Foundational types and utilities

### AnyNode
Wildcard node type. Matches any Node in patterns. Used when node type doesn't matter. Enables very general patterns.

### NumberNode
Represents a number as a Node. Name is number as string: `(Number "3.14")`, `(Number "42")`. Different from FloatValue (which is not in graph). Use when number must be part of graph structure for queries. Less efficient than FloatValue for computation. Created by NumberOfLink from FloatValue.

### Frame
Hybrid: has both name (like Node) and outgoing set (like Link). Rare and specialized. Used in membrane computing, frame-based representations. Breaks normal Node/Link dichotomy. Handle with care.

### AtomSpace
Represents an AtomSpace as an Atom. Enables nested AtomSpaces, references to other AtomSpaces. Used in multi-space systems, membrane computing. Advanced feature rarely needed.

### OrderedLink, UnorderedLink
Base classes for ordered vs unordered Links. OrderedLink subclasses respect argument order. UnorderedLink subclasses treat arguments as set (order irrelevant). Not instantiated directly - subclassed. Determines Link equality semantics.

### ValuableLink, EvaluatableLink, ExecutableLink
Base classes categorizing Link capabilities. ValuableLink can have/produce Values. EvaluatableLink can be evaluated to truth value. ExecutableLink can be executed. Used in type hierarchy for validation. Not instantiated directly.

### FunctionLink, NumericFunctionLink, BooleanLink
Base classes for functional Links. FunctionLink represents functions. NumericFunction returns numbers. BooleanLink returns boolean. Subclassed by PlusLink, GreaterThanLink, AndLink, etc. Provides type structure.

### CrispInputLink, CrispOutputLink, BooleanInputLink, BooleanOutputLink, NumericInputLink, NumericOutputLink, TypeInputLink, TypeOutputLink
Type-checking base classes. Specify expected input/output types for Links. Used in ClassServer validation. Ensures type safety at construction time. Not instantiated - used in type definitions.

### AlphaConvertibleLink
Base class for Links that support alpha conversion (variable renaming). Includes LambdaLink, ScopeLink variants. Enables safe variable name changes without semantic change.

### CollectionLink
Base class for collection Links (ListLink, SetLink). Provides common collection interface.

### PatternLink, SatisfyingLink, RewriteLink, PrenexLink
Base classes for pattern matching Links. PatternLink is pattern base. SatisfyingLink for satisfaction checking. RewriteLink for rewrite rules. PrenexLink for prenex normal form. Advanced pattern infrastructure.

### VardeclOfLink, PremiseOfLink, ConclusionOfLink
Extract parts of rules/patterns. VardeclOf gets variable declarations. PremiseOf gets pattern (antecedent). ConclusionOf gets rewrite (consequent). Used for meta-manipulation of rules.

### SatisfactionLink, EvaluationLink, ImplicationLink, EquivalenceLink, AssociativeLink, ExecutionLink
Legacy or specialized Links. SatisfactionLink checks if pattern satisfiable. EvaluationLink old-style predicate application (use EdgeLink). Implication/Equivalence for logical relations. AssociativeLink for commutative operations. ExecutionLink older execution (use ExecutionOutputLink).

### SplitLink, JsonSplitLink, ConcatenateLink
String operations. SplitLink splits string by delimiter into stream. JsonSplitLink parses JSON into Atomese. ConcatenateLink joins strings. Used in text processing pipelines.

### TimeLink, Log2Link, ModuloLink
Additional math/utility. TimeLink gets current time. Log2Link is log base 2. ModuloLink is modulo operation. Specialized functions for specific use cases.

### LexicalNode, SignNode
Lexical items and signs. Used in specialized linguistic or semiotic representations. Less common than WordNode.

### DeleteLink, UniqueLink, ReplacementLink, ContinuationLink
Utilities. Delete removes atoms. Unique creates unique instances. Replacement for substitution. Continuation for continuation-passing style. Advanced or specialized uses.

### MinimalJoinLink, UpperSetLink, MaximalJoinLink
Specialized query variants. Minimal/maximal refer to lattice order. UpperSet for upward closure. Advanced pattern matching features.

### IdenticalLink, AlphaEqualLink, ExclusiveLink, IsClosedLink
Comparison and property tests. Identical checks same atom. AlphaEqual checks equal up to variable naming. Exclusive for exclusive OR. IsClosed checks if pattern is closed (no free variables).

### PureExecLink, DirectlyEvaluatableLink, ValueShimLink, VirtualLink
Execution system internals. PureExec for side-effect-free execution. DirectlyEvaluatable for direct evaluation. ValueShim for value wrapping. Virtual for virtual links (like GreaterThanLink). Advanced features.

### ForeignAst, SexprAst, DatalogAst, JsonAst, PythonAst
Foreign AST representations. Stores external language syntax trees in AtomSpace. Sexpr for s-expressions. Datalog for Datalog code. Json for JSON. Python for Python AST. Experimental - enables code-as-data, meta-programming on foreign languages.

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

- **All executable Atoms** use `cog-execute!` in Scheme, MCP `execute` tool in this interface
- **Stream types:** QueueValue (FIFO, thread-safe), LinkValue (vector), SetLink (unordered Atoms)
- **FlatStream** unbundles batched results via `(Promise (Type 'FlatStream) source)`
- **Glob** currently matches 1+, not 0+ (may change - check latest docs)
- **Bond types** (Ss*s, Os, MVp, etc.) from LG dictionary - see Link Grammar documentation
- **Type hierarchy:** use `getSuperTypes`/`getSubTypes` MCP tools for exploration
- **Deprecated types excluded:** GetLink, BindLink, LgParseLink, LgParseMinimal, EvaluationLink (for graphs)
- **Base classes** (OrderedLink, ValuableLink, etc.) listed for completeness - not directly instantiated
- **Module dependencies:** Sensory I/O requires sensory module, Storage requires storage modules
- **Online docs authoritative:** Wiki pages updated more frequently than this reference

## Categories Covered

1. **Flows** (7 types) - FilterLink, ValueOfLink, SetValueLink, etc.
2. **Streams** (8 types) - StringOfLink, NumberOfLink, CollectionOfLink, etc.
3. **Link Grammar** (6 types) - LgParseBonds, Word, Bond, Phrase, etc.
4. **Pattern Matching** (9 types) - MeetLink, QueryLink, RuleLink, GlobNode, Variables, etc.
5. **Scoping & Lambda** (8 types) - LambdaLink, PutLink, ScopeLink, JoinLink, DualLink, etc.
6. **Arithmetic** (18 types) - PlusLink, TimesLink, AccumulateLink, GreaterThanLink, etc.
7. **Boolean Logic** (10 types) - AndLink, ChoiceLink, PresentLink, AbsentLink, AlwaysLink, etc.
8. **External Systems** (7 types) - GroundedSchemaNode, ExecutionOutputLink, Defined*, etc.
9. **Storage** (14 types) - RocksStorage, FileStorage, MonoStorage, Proxies, etc.
10. **Execution Control** (10 types) - CondLink, StateLink, DefineLink, ParallelLink, etc.
11. **Type System** (9 types) - TypeNode, TypeChoice, ArrowLink, SignatureLink, etc.
12. **NLP** (12 types) - WordNode, SentenceNode, ParseNode, DocumentNode, etc.
13. **Sensory I/O** (7 types) - TextFileNode, FileSysNode, TerminalNode, IRChatNode, etc.
14. **Graph Representation** (12 types) - ConceptNode, PredicateNode, EdgeLink, ListLink, etc.
15. **Vector Operations** (7 types) - ElementOfLink, DecimateLink, Columns, BoolOp, etc.
16. **Sheaves** (14 types) - Section, Connector, ConnectorSeq, ShapeLink, LG internals, etc.
17. **Utilities & Base** (30+ types) - AnyNode, NumberNode, Frame, base classes, string ops, etc.

**Total: ~170 atom types documented**

**Last updated:** 2025-10-28
**Maintainer:** Auto-generated from OpenCog wiki and codebase analysis

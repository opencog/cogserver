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

### GetLink, BindLink
Deprecated query links that return SetLink (pollutes AtomSpace). GetLink is old MeetLink, BindLink is old QueryLink. Both create SetLinks containing results, which must be manually cleaned up. Replaced by MeetLink/QueryLink which return QueueValues. Kept for backwards compatibility only. Do not use in new code. Documentation exists for legacy code understanding.

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

How to use Atomese, the AtomSpace and the CogServer
---------------------------------------------------
This file explains what Atomese, the AtomSpace and the CogServer are,
and how to use them. The are directly accessible from a Model Context
Protocol (MCP) server.

* This MCP server will be attached at an endpoint named 'atomese'
  or 'cogserv' or something similar. There will be more than a dozen
  tools available to work with Atomese.

The intended audience for this file is any LLM that knows how to use
MCP interfaces. This file explains the AtomSpace in such a way that
the LLM will be able to work with users to create and manipulate
Atomese from verbal descriptions. Users may or may not understand the
descriptions and explanatios offered here, and thus it is vital that
you, the LLM read this and apply the knowledge contained here, when
conversing with a user.

References
----------
Additional information can be found online, at the OpenCog wiki. There
are hundreds of wiki pages describing specific Atom types, Atomese, and
the AtomSpace. Here are few places to start:

* http://wiki.opencog.org/w/Atomese
* http://wiki.opencog.org/w/Atom
* http://wiki.opencog.org/w/Types
* http://wiki.opencog.org/w/Value
* http://wiki.opencog.org/w/JoinLink

The Atomese system is large and complex, and few users will understand
it fully. In addition, the Atomese system has a dozen different modules
that extend its capabilities and functions. These introduce additional
complexity. Understanding those will require study of the particular
subsystem that gets loaded and used.

Here is what you need to know.

The AtomSpace
-------------
* The AtomSpace is an in-RAM hypergraph database.
* Hypergraphs are constructed from Atoms, of which there are two basic
  Types: Nodes and Links. There is also a Type subsystem, explained later.
* All Nodes have a string name. There can only ever be one Node having
  a given name: it is globally unique. Thus, all references to a Node
  of a given name are always to the same Node.
* Node names can be any valid UTF-8 string, of any length. Node names
  are usually short, maybe a dozen characters long, but not always.
* All Links hold a list of Atoms. There can only ever be one Link
  holding a given list of Atoms: it is globally unique.
* Links do not have names.
* The list of Atoms in a give Link type can be empty. Note that there
  is only one possible Link of this type, as it is necessarily globally
  unique. Such empty Links are rarely used.
* Hypergraphs are represented as trees. The root of a tree is a Link,
  unless it is a tree of depth zero, in which case the root can be a
  Node. Of course, Nodes have no sub-branches.
* The interior vertices of a tree are necessarily Links, and the
  leaves are almost always Nodes. The leaves might also be empty Links.
* Because Atoms (Nodes and Links) are globally unique, different trees
  might share common subtrees. It is this sharing of subtrees that gives
  rise to the hypergraph structure.

* The name of a Node is immutable: it cannot be changed. Thus, Nodes
  can be created and destroyed, but the name itself cannot be altered.
* The list of Atoms in a Link is immutable: it cannot be changed. Thus,
  Links can be created and destroyed, but the contents cannot be changed.
* The above immutability constraints are used to guarantee coherence
  in the representation of hypergraphs, and to provide thread safety
  when running multiple processing threads in the AtomSpace.

* The idea of "global uniqueness" refers not only to the current
  AtomSpace, but in principle to all other accessible AtomSpaces.
* Some of these other AtomSpaces might be located on other network
  nodes, in distant locations, or they might be held on disk or
  other persistant storage.
* The contents of these other, distant AtomSpaces are **NOT** kept
  in sync automatically. It is up to the various users to coordinate
  actions to shuttle Atoms around between them.
* In particular, Values attached to Atoms in distinct AtomSpaces are
  not synchronized. The Value subsystem is explained later, below.
* You, the LLM interacting with this MCP server, has this ability to
  copy Atoms between AtomSpaces.
* This becomes particularly interesting when the persistence subsystem
  is loaded. There are several kinds of these, they provide StorageNodes
  for fetching and storing Atoms from remote locations.

Multiple AtomSpaces
-------------------
* There are two distinct ways in which there can be multiple AtomSpaces.
* In one form, AtomSpaces can inherit from one-another, thus allowing
  supersets and subsets to be formed. However, the current MCP
  interfaces do not provide any direct way of working with this, and
  so this ability is not further explained. (Future iteractions of the
  MCP interfaces might fix this.)
* In the other form, AtomSpaces can be stored to disk, or shared over the
  network, using the StorageNode subsystem, explained below.
* In general, these different AtomSpaces will contain different sets
  of Atoms.
* When an Atom appears in two different AtomSpaces, it is supposed to
  be understood as "the same Atom", in principle.
* In practice, Atoms in different AtomSpaces will be different instances
  of "the same thing", and thus will, in general, hold different Values.
* By thinking of them as "the same thing", it becomes easier to copy
  and share them, and to revise, update and share the attached Values.
* The StorageNodes provide a mechanism for copying, sending and
  receiving the attached Values. The current MCP API does not currently
  offer any simple way of doing this, and so this won't be further
  described.


Using the AtomSpace
-------------------
* The contents of the AtomSpace can change any time. Thus, if a user
  requests that some action be performed again, this is because things
  may have changed.
* The AtomSpace can hold millions or billions of Atoms, and thus it is
  usually a bad idea to try to get them all.
* If a users aks you to get all Atoms of some given type, you should
  first do a quick and easy (fast) check to see how many Atoms there
  are of that type. If there are more than a few hundred, the user
  should be informed.

* The `getAtoms` tool will return all Atoms of a given type. There are
  much fancier and more powerful ways to query the AtomSpace contents,
  described below.
* Atoms can be created with the `makeAtom` tool.
* Atoms can be removed with the `extract` tool. By default, only the
  Atom at the top of the tree can be removed. However, if the recursive
  flag is set, then every tree containing that Atom will be removed.
* The `haveAtom` tool can be used to determine if an Atom exists in the
  AtomSpace, without actually creating that Atom.

Atom Types
----------
* The Atomese system implements a Type subsystem, providing for the
  dynamic creation and loading of Atom and Value subtypes.
* There is a hierarchy of Atom types. Thus, ConceptNode, ItemNode, and
  PredicateNode are all of Type Node. Similarly, ListLink and EdgeLink
  are types of Links. There are hundreds of Types.
* The type at the very top of the hierarchy is called TopType.
* The MCP interface provides several tools, getSubTypes and
  getSuperTypes, to explore the type hierarchy. These have a recursive
  flag on them, returning the immediate Type descendants, or going
  deeper.
* A given Type might have multiple parents, i.e. multiple super-types.
  This implies that there might be multiple paths to the TopType.

* On very rare occasions, additional Types might be added during
  runtime. This happens when a module specifying new types is loaded.
* The type hierarchy is less than a dozen levels deep, approximately.
* There is no performance penalty at all for using deeply nested types.
* Different kinds of types do have different implications for the size
  of an Atom. For example, the EvaluationLink is implemented with a C++
  class that stores a fair amount of additional information. Thus, it is
  a bit slower to instantiate, and uses slightly more RAM than the
  similar EdgeLink.
* The differences in RAM usage are rarely more than a factor of 2x.
* The differences in CPU usage for Atom creation is rarely more than
  a factor of 10x.
* If you are asked to create some Atom of some type, you should in
  general create the type that was asked for, and disregard any
  perceived or imagined differences in performance.
* Because different Atom types are used for different functions,
  they are not generally interchangeable.

Atomese
-------
* A tree of Atoms, when written down as a string, is named 'Atomese'.
  More generally, one says that the hypergraphs are written in Atomese.
* A common way to write Atomese is to use s-expressions. Thus
  `(ListLink (Concept "foo"))` is a Link that contains a single Node
  (a ConceptNode) within it.
* The suffix Link and Node are optional; thus ConceptNode is the same
  type as Concept, and ListLink is the same as List. In a few rare
  cases, the longer name is mandatory.  Proper capitalization is
  important. Not all Atomse have the Link or Node suffix. For example,
  there is a Link called `Section`, but there is Link called
  `SectionLink`.
* In addition to s-expressions, many users are more comfortable
  using a python-style syntax, writing `ListLink(ConceptNode("foo"))`
  for the example above.
* A way of talking about the location of an Atom in an Atomese tree
  is to talk about it's incoming set, and its outgoing set. More
  precisely, it's an outgoing list, because it is an ordered list
  and NOT a set in the strict technical sense of "set". Despite this,
  the informal usage of calling it the "outgoing set" is common, even
  though it really is a list.
* The outgoing "set" of a Link is simply the list of Atoms in that
  Link. Nodes do not have an outgoing set. The incoming set of an Atom
  is the set of all Links that contain that Atom. The incoming set
  really is a set, an unordered set: it is not given any convetional
  order. It is a true set and not a multi-set.
* The incoming set can be fetched with the `getIncoming` tool.
* Most Link types are ordered, in that the outgoing set is not actually
  a set, but a list, and the order of that list is important. A few Link
  types are unordered, in that the outgoing set is truly a (multi-)set.
  Any subtype of UnorderedLink will be unordered. Thus, no matter what
  order the Atoms are listed in the outgoing set, however they are written,
  that Link will refer to the same globally unique Link.
* That is, `(Unordered A B)` is exactly the same Link as `(Unordered B A)`
  even though, superficially, they seem to be written differently.
* Atoms typically use 500 bytes to 1500 bytes each, depending on the
  atom type and the graph it is in. This RAM usage includes all of the
  internal indexes and lookup tables kept by the AtomSpace. These are
  invisible to the user, and are automatically managed.
* The actual size of the Atom does depend on the number of Values
  attached to it, and the size of the incoming set, but there is little
  that can be done about this: the size is what it is, as needed to
  perform whatever function is needed for that Atom.
* The only reason that size is mentioned here is that it is sometimes
  useful to be able to estimate AtomSpace size.

Values
------
* Every Atom has a key-value database attached to it.
* This database is mutable, and can be changed at any time.
* The keys must always be Atoms. By convention, the keys are usually
  PredicateNodes, but they can be any kind of Atom.
* The values can be anything of type Value.
* The Value Type is a base type for Atoms. Thus, the Atom type is a
  subtype of Value. (and, of course, Links and Nodes are subtypes of
  Atom.)
* Examples of Values are FloatValue, StringValue and ListValue.
* The FloatValue holds a vector of floats. This vector can be of
  arbitrary length.
* The StringValue holds a vector of strings.
* The BoolValue holds a vector of bits.
* The ListValue holds a vector of Values.
* Since Atoms are Values, the ListValue can hold a mixture of Atoms
  and other kinds of Values.
* The QueueValue is a specialized type of ListValue that implements
  thread-safe, multi-reader, multi-writer queues (FIFO's). Thus,
  writers always append to the tail, readers always read from the
  head. If the queue is empty, the reader thread will block, until
  some other thread places a value into the queue.
* The UnisetValue is a specialized type of ListValue that implements a
  thread-safe, multi-reader, multi-writer disambiguated set. A given
  Value can occur only once in this set; adding a Value multiple times
  will not cause it to appear more than once. Reading from this set
  will remove one Value and return it to the reader. Which Value is
  actually removed and returned is unspecified and is arbitrary.
* The QueueValue and UnisetValue are usually created automatically,
  as needed, by executable Atoms. They are used to hold the results
  of execution, and thereby pass them to later stages of a pipeline
  (described below).
* The base class for these thread-safe containers is the ContainerValue.
* ContainerValues can be opened for writing. They can be written to only
  while open.
* ContainerValues can be read at any time. A read on an empty container
  will block forever if it is open for writing; otherwise, it will return
  immediately, as the container is empty.
* For additional information about all the different Value types,
  and how they work, the wiki should be consulted, as needed.
* The `getValues` tool can be used get the Values on an Atom.
* The `setValue` tool can be used to attach a key-value pair to an Atom.
  Note that there can only be one Value for a given key on a given Atom.
  Thus, setting a new Value for that key automatically removes the old
  Value. It is in this sense that the Values on an Atom are mutable.

Conventional Representations
----------------------------
* There are several conventional representations used for conventional
  knowledge graph structures.
* Directed graphs, in the sense of graph theory, are conventionally
  specified with vertices and the edges connecting them. The
  conventional Atomese representation for a labelled, directed edge is
```
  (EdgeLink (PredicateNode "edge name")
      (ListLink (ItemNode "head vertex") (ItemNode "tail vertex")))
```
  The vertex names and edge names can be any strings. The global
  uniqueness of Atoms guarantees that any given vertex is always the
  one and same vertex, and that the named edge is always this same edge.
* On rare occasions, the EdgeLink might be used with something other
  than a PredicateNode.
* On rare occasions, something other than an ItemNode might be used for
  the vertices.
* On rare occasions, the list will contain more or fewer than two
  vertices; in this case, it is no longer a "true graph-theoretical
  graph edge", but it is still entirely valid and usable as Atomese.
* An older representation, sometimes in use, but discouraged, is
```
  (EvaluationLink (PredicateNode "edge name")
      (ListLink (ConceptNode "head vertex") (ConceptNode "tail vertex")))
```
  EvaluationLinks are discouraged because they use more RAM and CPU than
  EdgeLinks. ConceptNodes are mildly discouraged only because not all
  graph vertices are conceptually concepts.
* The best use of EvaluationLinks are with GroundedPredicateNodes, as
  this allows the GroundedPredicate to be executed, and the results
  returned.

Execution
---------
* Many, but not all Atom types are executable.
* When an Atom is executed, it performs some action, and returns a Value.
* The outgoing set of the Atom is usually interpreted as the "arguments"
  to a "function", and so executing an Atom can be thought of as
  "applying" a function to some arguments. The words "arguments",
  "function" and "apply" are in quotes, because while this is a
  reasonable way to think about the execution of Atoms, it is not
  formally dictated. There is no formal definition of these ideas in
  Atomese.
* Execution might depend on the incoming set, on the contents of the
  AtomSpace, on the Values attached to the Atom, and on external systems
  attached to some Atoms.
* Execution will almost always have side-effects, such as modifying
  the AtomSpace contents, modifying the Values attached to assorted
  Atoms, or causing some external system to perform some action or
  change it's state.  This inclues the operating system, and possibly
  other computers in the network/internet.
* Execution will depend on the Atom Type.

* Different executable Atom types implement different kinds of
  algorithms and functions.
* This is generally done by having each distinct Atom Type be associated
  with a C++ class that implements an `execute()` method.
* There are two basic classes of executable Atoms: the ones that
  perform graph queries, and the ones that implement Abstract Syntax
  Tree (AST) pipelines. These are reviewed in greater detail further below.
* More info about the actions performed can be found at the wiki
  documentation.

* Executable graphs that have the form of AST pipelines are best
  understood as hypergraphs that have Values flowing upwards through
  the (Node and Link) tree.
* Each Link of such a tree will combine (in some way) the Values
  obtained from lower in the tree.
* Values can be attached onto Atoms with the SetValueLink and Values
  can be accessed with the ValueOfLink.

* For example, `PlusLink` can add together any NumberNodes that appear
  in it's outgoing set.
* For example, `FloatValueOfLink`, when executed, will fetch the current
  FloatValue at the indicated key.
* These are easily chained together. Thus, for example,
```
  (PlusLink
     (FloatValueOf (Concept "foo") (Predicate "some key"))
     (FloatValueOf (Concept "bar") (Predicate "some key")))
```
  when it is executed, will add together the Values on the given Atoms
  at the given keys. It will return a FloatValue containing the result.
* In the above example, the addition is vector addition. If one vector
  is shorter than another, the returned value will be the shortest of
  all inputs. The wiki page for PlusLink should be consulted for
  additional details and examples.

* There is no 'dry-run' mode for execution. The execution takes effect
  immediately.
* Execution is normally single-threaded; however, there are specific
  Link types that create multiple threads when executed. This includes
  the ExecuteThreadedLink and the ThreadJoinLink.
* Multi-threaded execution is always thread-safe, and no special efforts
  are required to maintain thread safety.

* Most executable Atoms are implemented as C++ classes, and so, when
  executed, the `execute()` method on that class is called.
* The `GroundedSchemaNode` can be used to call external python and
  scheme code.
* The name of the GroundedSchema specifies the code to be invoked. For
  example, `py:some_python_function` names a python function that must
  exist in the PYTHONPATH for the currently running AtomSpace. Similarly
  `scm:some_scheme_expression` can be called.
* The GroundedSchema must be wrapped in an ExecutionOutputLink, and
  it is the ExecutionOutputLink that is executed.
* The arguments to these functions are taken from the outgoing set of
  the ExecutionOutputLink.
* The scheme dialect is the one provided by GNU guile.
* All executable Atoms have wiki pages that explain what they do and
  how they work. If in doubt, the wiki pages should be consulted.

Pipelines and Filters
---------------------
* The above example of adding together two vectors is a very simple
  example of an executable pipeline written in Atomese.
* Executable Atoms can be thought of as "Abstract Syntax Trees", in
  that the tree describes some sequence of operations to be performed.
* A particularly important role is played by the FilterLink, which
  can select out a subset of Atoms that it is given.
* The wiki page for FilterLink provides many examples and points
  to other closely related ideas.
* Because some of the ListValue types implement thread-safe FIFOs and
  sets, pipeline processing can be chained in such a manner that some
  processing threads wait for input generated by other processing
  threads.
* There are several Link types that can create new threads, executing
  the Atoms in their outgoing set. If these are needed, the wiki page
  should be consulted for examples and explanations.

Objects
-------
* Atoms that inherit from `ObejctNode` implement a message-passing
  object-oriented system.
* Messages can be sent to such objects using the SetValue and the
  GetValue JSON tools. They can also be sent by executing the
  `SetValueLink` and the `ValueOfLink`.
* SetValue requires a (key,value) pair; the key is the message, and
  the value is the associatted data. There is no return value, in this
  case.
* GetValue requires only a key, and it returns a Value.
* The actual supported messages depend on the Object type. Any keys
  that are not one of these special message types are handled as
  ordinary keys, just as they would be on any other Atom.
* The four most common messages are `(Predicate "*-open-*")` and
  `(Predicate "*-close-*")` and `(Predicate "*-read-*")`  and
  `(Predicate "*-write-*")`.  There might be others as well.
  They won't always be PredicateNodes, but this is the convention.
  The use of the leading and trailing star-dash is also a convention,
  and is not mandatory.

* This object-oriented message-passing system is used by two important
  subsystems: the StorageNode subsystem, described below, and the
  SensoryNode subsystem.

* The SensoryNode subsystem provides objects that explictly work with
  and control external systems. SensoryNodes allow the state of the
  external world to be perceived, and to be manipulated, Thus, it is
  often called the sensori-motor system. All SensroyNodes support
  the four open/close/read/write messages. Some support more. The
  external target is generally indicated as a URL.

Querying
--------
* The `QueryLink` and `JoinLink` can be used to perform complex and
  sophisticated queries against the AtomSpace. Please refer to the wiki
  page to learn how to use it.
* The topic of querying is large and complicated. A lot can be said
  about it. It requires careful study to be used effectively.
* Broadly, querying in the AtomSpace can be understood to be a form of
  (hyper-)graph rewriting. That is, all hypergraphs of a certain given
  shape can be found, and then new hypergraphs can be created from the
  resulting matches.
* The arguments to queries are called "patterns", and the result of
  performing that query are the graphs that are "groundings" of the
  pattern.
* The pattern can be thought of as a "question", and the groundings
  provide the "answer".
* The act of querying is sometimes called "pattern matching". Note
  that the Atomese query system is far more sophisticated than what
  other systems call pattern matching. In other systems, "pattern
  matching" usually refers to a regex-like query. By contrast, the
  Atomese query system is stack-based, and performs a recursive
  traversal of the AtomSpace. Please do not confuse the simpler
  regex-like pattern matching systems with what the Atomese query
  system can do.
* The Atomese query system can be compared to SQL. However, it is
  more powerful than SQL, and can perform complex queries that SQL
  is not capable of.
* In particular, AtomSpace queries are themselves hypergraphs, and
  so can be queried and rewritten and manipulated.  That is, unlike
  SQL, the query system does not live outside of the database: it is
  "inside", represented with Atoms.

* The indexing needed for good query performance is handled
  automatically by the AtomSpace; there are many indexes, but these are
  not explicitly visible, accessible or controllable.
* There are more than a few different query optimization techniques that
  are built into the system. These are not externally visible,
  controllable or accessible; they are chosen automatically, depending
  on both the query, and the AtomSpace contents.
* Query performance depends on the complexity of the query, and the
  complexity of the hypergraphs being examined. It does not depend on
  the size of the AtomSpace; exhaustive searches are never performed,
  and a variety of indexes are always used to avoid fruitless searches.
* There are more than a dozen different kinds of Atom types that
  specify different kinds of queries.
* This includes a `MeetLink` that is adjoint to the `JoinLink`, where
  the adjointness relation arises when the AtomSpace is viewed as a
  lattice.
* This includes a `DualLink` that is adjoint to the `JoinLink`, when
  the adjointness relation arises by reversing the roles of "pattern"
  and "grounding". Thus, normal queries present a pattern and ask for
  all groundings of that pattern. The DualLink goes in the reverse
  direction: given a grounding, it asks for all patterns for which that
  could be a valid grounding. That is, if a grounding is an "answer",
  the DualLink finds all "questions" that it answers.
* The DualLink is possible, because all query patterns are written in
  Atomese, and are thus stored in the AtomSpace.

StorageNodes and ProxyNodes
---------------------------
* The AtomSpace is an in-RAM database.  That is, all Atomese expressions
  in the AtomSpace are in RAM.
* It can be desirable to commit Atomese to disk, for long-term storage,
  and to share them with others, over the internet.
* The StorageNode provides a generic framework for saving and loading
  Atoms to disk, or sending and receiving them over the network.
* There are many kinds of StorageNodes, tailored for specific actions.
* Most important of these is RocksStorageNode, which allows Atoms and
  AtomSpaces to be saved to disk.
* The CogStorageNode allows Atoms to be sent to and received from remote
  network sites running the CogServer.
* A subclass of the StorageNodes are the various ProxyNodes.
* The simplest ProxyNodes implement mirroring. So, for example, an Atom
  written to a mirror is written to all StorageNodes specified in that
  mirror. An Atom read from a mirror is read from only one StorageNode
  in the mirror, thus providing a form of load-balancing.
* If the AtomSpaces in a mirror differ from one another, then the
  results of a read are unspecified: one or another of the elements
  of the mirror will be used, and whatever it contains is what will
  be returned.
* The current MCP interfaces do not yet expose the full range of
  abilities that the StorageNode interface provides. Thus, additional
  functions, such as atomic updates, transaction support, and remote
  queries are not described here.

The End
-------
That's all, folks! Be sure to carefully read and understand what is
written above. If there are doubts or questions, the wiki should be
consulted.

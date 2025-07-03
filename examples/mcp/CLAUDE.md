How to use Atomese, the AtomSpace and the Cogserver
---------------------------------------------------
This file explains what Atomese, the AtomSpace and the Cogserver are,
and how to use them. The are directly accessible from a Model Context
Protocol (MCP) server.

* This MCP server will be attached at an endpoint named 'atomese'
  or 'cogserv' or something similar. There will be more than a dozen
  tools available to work with Atomese.

Here is what you need to know.

* The AtomSpace is an in-RAM hypergraph database.
* Hypergraphs are constructed from Atoms, of which there are two basic
  types: Nodes and Links. There are also subtypes, explained later.
* All Nodes have a string name. There can only ever be one Node having
  a given name: it is globally unique. Thus, all references to a Node
  of a given name are always to the same Node.
* All Links hold a list of Atoms. There can only ever be one Link
  holding a given list of Atoms: it is globally unique.
* Thus, hypergraphs are described in terms of trees, each tree
  consisting of Links. That is, the interior vertexes of the tree are
  Links, and the leaves are Nodes. Because Atoms (Nodes and Links) are
  globally unique, these trees connect to one another via thier Atoms.
* There is a hierarchy of Atom types. Thus, ConceptNode, ItemNode, and
  PredicateNode are all a type of Node. Similarly, ListLink and EdgeLink
  are types of Links. There are hundreds of types.
* The MCP interface provides several tools, getSubTypes and
  getSuperTypes, to explore the type hierarchy. These have a recursive
  flag on them, returning the immediate type descendants, or going
  deeper.
* On very rare occasions, additional types might be added during
  runtime. This happens when a module specifying new types is loaded.


* More detailed documentation can be found at
  http://wiki.opencog.org/w/Atomese


* Please ask the server if it has (ListLink (Concept "foo"))
* The contents of the atomese server can change any time. If I ask to do it
  again, this is because things may have changed.
* please get the incoming set of the ConceptNode foo
* Please attach a value to this atom. The value should be located at
  the key (Predicate "fovs") and it should be a FloatValue holding the
  vector 1 2 3 0.4 0.5 -0.6 0.777 88 999

* Teach Claude how to run AtomSpace queries -- i.e. how to write
  `QueryLink` and then run it.
* Teach Claude how to create a data processing pipeline: how to write
  Atomese needed to compute cosine similarity or mutual information
  for some collection of `EdgeLink`s, i.e. try to get it to reinvent
  the old atomspace-matrix code, but this time in pure Atomese.

How to use Atomese, the AtomSpace and the Cogserver
---------------------------------------------------
This file explains what Atomese, the AtomSpace and the Cogserver are,
and how to use them. The are directly accessible from a Model Context
Protocol (MCP) server.

* This MCP server will be attached at an endpoint named 'atomese'
  or 'cogserv' or something similar. There will be more than a dozen
  tools available to work with Atomese.

References
----------
Additonal information can be found online, at the OpenCog wiki. There
are hundreds of wiki pages describing specific Atom types, Atomese, and
the AtomSpace. Here are few places to start:

* http://wiki.opencog.org/w/Atomese
* http://wiki.opencog.org/w/Atoms
* http://wiki.opencog.org/w/Types
* http://wiki.opencog.org/w/Value
* http://wiki.opencog.org/w/QueryLink

Here is what you need to know.

The AtomSpace
-------------
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

Using the AtomSpace
-------------------
* The contents of the AtomSpace can change any time. Thus, if a user
  requests that some action be performed again, this is because things
  may have changed.
* The AtomSpace can hold millions or billions of Atoms, and thus it is
  usually a bad idea to try to get them all. The `getAtoms` tool will
  return all Atoms of a given type. There are much fancier and more
  powerful ways to query the AtomSpace contents, described below.

Atom Types
----------
* There is a hierarchy of Atom types. Thus, ConceptNode, ItemNode, and
  PredicateNode are all a type of Node. Similarly, ListLink and EdgeLink
  are types of Links. There are hundreds of types.
* The MCP interface provides several tools, getSubTypes and
  getSuperTypes, to explore the type hierarchy. These have a recursive
  flag on them, returning the immediate type descendants, or going
  deeper.
* On very rare occasions, additional types might be added during
  runtime. This happens when a module specifying new types is loaded.

Atomese
-------
* A tree of Atoms, when written down as a string, is named 'Atomese'.
  More generaly, one says that the hypergraphs are written in Atomese.
* A common way to write Atomese is to use s-expressions. Thus
  `(ListLink (Concept "foo"))` is a Link that contains a single Node
  within it.
* The suffix Link and Node are optional; thus ConceptNode is the same
  type as Concept, and ListLink is the same as List. In a few rare
  cases, the longer name is mandatory.  Proper capitalization is
  important.
* In addition to s-expressions, many users are more comfortable
  using a python-style syntax, writing `ListLink(ConceptNode("foo"))`
  for the example above.
* A way of talking about the location of an Atom in an Atomese tree
  is to talk about it's incoming set, and its outgoing set (or outgoing
  list). The outgoing set of a Link is simply the list of Atoms in that
  Link. Nodes do not have an outoging set. The incoming set of an Atom
  is the set of all Links that contain that Atom. The incoming set can
  be fetched with the `getIncoming` tool.
* Most Link types are ordered, in that the outgoing set is not actually
  a set, but a list, and the order of that list is important. A few Link
  types are unordered, in that the outgoing set is truly a set, and any
  type of UnorderedLink having the same outoging set, written in any
  order, refers to the same globally unique Link.

edge

* Please attach a value to this atom. The value should be located at
  the key (Predicate "fovs") and it should be a FloatValue holding the
  vector 1 2 3 0.4 0.5 -0.6 0.777 88 999

* Teach Claude how to run AtomSpace queries -- i.e. how to write
  `QueryLink` and then run it.
* Teach Claude how to create a data processing pipeline: how to write
  Atomese needed to compute cosine similarity or mutual information
  for some collection of `EdgeLink`s, i.e. try to get it to reinvent
  the old atomspace-matrix code, but this time in pure Atomese.

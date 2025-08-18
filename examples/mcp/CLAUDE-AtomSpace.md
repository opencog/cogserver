
Information for Claude about how to use the AtomSpace and the Cogserver.
Please read this carefully. By understanding what is written below, it
will be easier to understand what a user is asking you to do.

* The Cogserver provides more than a dozen MCP tools that can be used to
  access and change the contents of an AtomSpace.
* The MCP server is called `cogserv`.

* The AtomSpace is an in-RAM database that can store Atoms.
* Atoms come in two core types: Nodes and Links.
* Nodes always have a name.
* Links are lists of Atoms.
* There are many different subtypes of Nodes and Links, and each of
  these may have more subtypes.
* The subtypes can be listed with one of the MCP tools.

* Atoms can be created and deleted using the MCP interface.
* The contents of the AtomSpace can be listed with MCP tools.
* The AtomSpce may contain millions or even billions of Atoms, so it is
  usually a bad idea to try to list all of them, unless you are
  explicitly asked to do so.
* Requests to list AtomSpace contents will usually indicate that only
  Atoms of a given type should be listed.

* The cogserver implements the MCP tools.
* The cogserver is attached to just one AtomSpace by default, but it
  can work with many, and the attached AtomSpace can change at any time.
* The cogserver is a multi-user system, and thus, the contents of an
  AtomSpace can change at any time.
* If you are asked to repeat doing something, youshould always assume
  that the contents of the AtomSpace changed, and thus that the task
  must be performed again.

* Some but not all atoms are executable.
* When executed, the contents of the AtomSpace may change.
* When executed, a collection of Values may be returned.

* Values can be lists of numbers (floats), lists of bits, lists of
  strings, or lists of other values.
* An Atom is a special case of a Value.

* Every Atom holds a key-value database.
* The key can be any Atom, but is usually a PredicateNode.
* The value can be any Value.
* The key-value pairs can be inserted, deleted or changed using the MCP
  tools.
* The list of all key-value pairs on an Atom can be obtained with an MCP
  tool.

* The AtomSpace can be used to store graphs.
* The canonical form for a graph edge is
  (EdgeLink (PredicateNode "name of edge")
      (ListLink (ItemNode "from-vertex") (ItemNode "to-vertex")))
* The names of the edge and the vertices can be any valid UTF8 string.

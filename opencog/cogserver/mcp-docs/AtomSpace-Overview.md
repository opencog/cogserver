
This text provides an introductory overview for MCP clients about the
AtomSpace and the Cogserver. Another text provides far more detailed
information, so please read this first.  Please read this carefully.
By understanding what is written below, it will be easier to understand
what a user is asking you to do.

* The CogServer is a (TCP/IP) network server that provides access to an
  AtomSpace instance, using a variety of different network interfaces.
* One of these interfaces is an MCP server interface. It can be accessed
  in raw form, at the MCP server port, or through a conventional http
  interface.
* The CogServer provides more than a dozen MCP tools that can be used to
  access and change the contents of an AtomSpace.
* The user can configure the MCP server to have any name, but commonly
  it will be called `atomese` or maybe `cogserv`.

* The AtomSpace is an in-RAM database that holds Atoms.
* Atoms come in two core Types: Nodes and Links.
* Nodes always have a name. These names can be any valid UTF-8 string,
  of any length. Names are usually quite short, usually shorter than
  a dozen characters.
* Links are always lists of Atoms, and do not have a name.
* There are many subtypes of Nodes and Links, and each of
  these may have more subtypes.
* The AtomSpace has a dynamic Type system, of which Nodes and Links
  are but two examples.
* There are hundreds of different Types.  The actual collection of
  Types depends on what modules are currently loaded. Different
  modules define additional types.
* The Types and thier subtypes can be listed with one of the MCP tools.

* Atoms can be created and deleted using the MCP interface.
* The contents of the AtomSpace can be listed with MCP tools.
* The AtomSpace may contain millions or even billions of Atoms,
  so it is usually a bad idea to try to list all of them, unless
  you are explicitly asked to do so.
* Requests to list AtomSpace contents will usually indicate that only
  Atoms of a certain given Type should be listed.
* Prior to listing contents, a quick, easy check should be made to see
  how many Atoms there are, of the requested type. If there are more
  than a few hundred, the user should be told how many there are, and
  asked to confirm.

* The CogServer implements the MCP tools.
* The CogServer is attached to just one AtomSpace by default, but it
  can work with many, and the attached AtomSpace can change at any time.
* AtomSpaces can be nested, so that one sits in another. They can also
  be joined and intersected. It is very rare that a user will ask you to
  do this, but it certainly is possible.
* The CogServer is a multi-user system, and thus, the contents of an
  AtomSpace can change at any time.
* If you are asked to repeat doing something, you should **always**
  assume that the contents of the AtomSpace have changed, and that this
  is why you are being asked to repeat the task.

* Some, but not all Atoms are executable.
* When executed, there might be any kind of side-effect, including
  changes and actions outside of the AtomSpace or CogServer.
* When executed, the contents of the AtomSpace may change.
* When executed, a collection of Values may be returned.

* Some Atoms provide an Object interface. Messages sent to Objects will
  typically change the contents of the AtomSpace.

* Values are a part of the AtomSpace Type subsystem.
* Values can be lists (vectors) of numbers (double-precision floats),
  lists of bits (bitvectors), lists of strings, or lists of other values.
* The Atom Type is a subtype of Value.

* Although all Atoms are Values, the revierse does not hold: not all
  Values are not Atoms. In particular, FloatValue,, StringValue and
  ListValue are not Atoms.
* A Link is a list of Atoms; only Atoms can be in that list. Values
  that aren't already Atoms cannot appear in that list.

* Every Atom holds a key-value database.
* The key can be any Atom, but is usually a PredicateNode.
* The value can be any Value.
* The key-value pairs can be inserted, deleted or changed using the MCP
  tools. These are the setValue and getValue tools.
* The list of all keys and all key-value pairs on an Atom can be obtained
  with an MCP tool.

* When an Atom is of type ObjectNode, then some of the keys are
  interpreted as message (method) names.
* Sending a message to an Object means setting or getting one of these
  special keys.
* Setting will set a (key,value) pair, getting will return a Value for
  that key.
* Getting of setting one of these special keys will cause the method
  associated with that message to run.
* When one of these methods runs, the contents of the AtomSpace might
  change, and other actions might be performed. Some of these may affect
  systems outside of the AtomSpace or Cogserver.

* The AtomSpace can be used to store graphs.
* A "graph" is understood to be a collection of vertexes and edges.
* The canonical form for a directed graph edge is
  (EdgeLink (PredicateNode "name of edge")
      (ListLink (ItemNode "from-vertex") (ItemNode "to-vertex")))
* The names of the edge and the vertices can be any valid UTF8 string.

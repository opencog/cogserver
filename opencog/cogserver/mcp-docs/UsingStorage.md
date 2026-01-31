# Using StorageNodes

This document explains how to use StorageNodes to persist Atoms to disk
or share them over the network via MCP.

## Overview

- The AtomSpace is an in-RAM database. StorageNodes allow saving and
  loading Atoms to/from persistent storage or remote systems.
- StorageNodes use a **message-passing API**: you send messages by calling
  `setValue` with special Predicate keys.
- Messages are always PredicateNodes with names like `*-open-*`, `*-store-atom-*`.
- The leading and trailing `*-` is a convention indicating these are messages.

## StorageNode Types

### RocksStorageNode
Saves Atoms to local disk using RocksDB.

**URL format**: `rocks:///path/to/database`

Note the three slashes: `rocks://` + `/path`. The path is an absolute filesystem
path where the RocksDB database directory will be created.

```json
{"atomese": "(RocksStorageNode \"rocks:///home/user/mydata.rdb\")"}
```

### CogStorageNode
Connects to a remote CogServer over the network.

**URL format**: `cog://hostname:port`

```json
{"atomese": "(CogStorageNode \"cog://localhost:17001\")"}
```

## Basic Workflow

Every StorageNode session follows this pattern:

1. **Create** the StorageNode
2. **Open** the connection
3. **Store/Load** atoms as needed
4. **Close** the connection

### Example: Complete Save and Load Cycle

```json
// Step 1: Create the StorageNode (using makeAtom tool)
{"atomese": "(RocksStorageNode \"rocks:///tmp/example.rdb\")"}

// Step 2: Open the connection (using setValue tool)
{
  "atomese": "(RocksStorageNode \"rocks:///tmp/example.rdb\")",
  "key": {"atomese": "(Predicate \"*-open-*\")"},
  "value": {"atomese": "(VoidValue)"}
}

// Step 3a: Create some data to store (using makeAtom tool)
{"atomese": "(Edge (Predicate \"likes\") (List (Concept \"Alice\") (Concept \"Bob\")))"}

// Step 3b: Store the atom (using setValue tool)
{
  "atomese": "(RocksStorageNode \"rocks:///tmp/example.rdb\")",
  "key": {"atomese": "(Predicate \"*-store-atom-*\")"},
  "value": {"atomese": "(Edge (Predicate \"likes\") (List (Concept \"Alice\") (Concept \"Bob\")))"}
}

// Step 4: Close the connection (using setValue tool)
{
  "atomese": "(RocksStorageNode \"rocks:///tmp/example.rdb\")",
  "key": {"atomese": "(Predicate \"*-close-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

## Message Reference

### Connection Messages

#### `*-open-*`
Opens a connection to storage. Must be called before any other operations.

**Argument**: `(VoidValue)`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-open-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

#### `*-close-*`
Closes the connection to storage. Always close when done.

**Argument**: `(VoidValue)`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-close-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

### Storing Messages

#### `*-store-atom-*`
Stores a single Atom and all Values attached to it.

**Argument**: The Atom to store

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-store-atom-*\")"},
  "value": {"atomese": "(Concept \"my-data\")"}
}
```

#### `*-store-atomspace-*`
Bulk stores the entire AtomSpace contents.

**Argument**: `(VoidValue)` (MCP uses the attached AtomSpace implicitly)

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-store-atomspace-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

#### `*-store-value-*`
Stores a specific Value on an Atom (without storing the whole Atom).

**Argument**: `(LinkValue (TheAtom) (TheKey))`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-store-value-*\")"},
  "value": {"atomese": "(LinkValue (Concept \"data\") (Predicate \"my-key\"))"}
}
```

### Loading Messages

#### `*-load-atomspace-*`
Bulk loads the entire contents of storage into the AtomSpace.

**Argument**: `(VoidValue)` (MCP uses the attached AtomSpace implicitly)

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-load-atomspace-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

#### `*-fetch-atom-*`
Fetches a specific Atom and all its Values from storage.

**Argument**: The Atom to fetch

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-fetch-atom-*\")"},
  "value": {"atomese": "(Concept \"my-data\")"}
}
```

#### `*-fetch-incoming-set-*`
Fetches all Links that contain a given Atom.

**Argument**: The Atom whose incoming set to fetch

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-fetch-incoming-set-*\")"},
  "value": {"atomese": "(Concept \"Alice\")"}
}
```

#### `*-fetch-incoming-by-type-*`
Fetches Links of a specific type that contain a given Atom.

**Argument**: `(LinkValue (TheAtom) (Type "LinkTypeName"))`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-fetch-incoming-by-type-*\")"},
  "value": {"atomese": "(LinkValue (Concept \"Alice\") (Type \"Edge\"))"}
}
```

#### `*-fetch-value-*`
Fetches a specific Value on an Atom.

**Argument**: `(LinkValue (TheAtom) (TheKey))`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-fetch-value-*\")"},
  "value": {"atomese": "(LinkValue (Concept \"data\") (Predicate \"my-key\"))"}
}
```

### Deletion Messages

#### `*-delete-*`
Removes an Atom from storage (but not from the AtomSpace).

**Argument**: The Atom to delete

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-delete-*\")"},
  "value": {"atomese": "(Concept \"obsolete-data\")"}
}
```

#### `*-delete-recursive-*`
Removes an Atom and all Links containing it from storage.

**Argument**: The Atom to delete recursively

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-delete-recursive-*\")"},
  "value": {"atomese": "(Concept \"obsolete-data\")"}
}
```

### Synchronization Messages

#### `*-barrier-*`
Ensures all pending operations are completed. Use for multi-threaded synchronization.

**Argument**: `(VoidValue)`

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-barrier-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

### Status Messages

These messages use `getValueAtKey` instead of `setValue`:

#### `*-connected?-*`
Check if the connection is open.

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-connected?-*\")"}
}
```

#### `*-monitor-*`
Get performance and debugging information.

```json
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/db\")",
  "key": {"atomese": "(Predicate \"*-monitor-*\")"}
}
```

## Opening Existing Datasets

To work with an existing RocksDB dataset:

```json
// Create the StorageNode pointing to the existing database
{"atomese": "(RocksStorageNode \"rocks:///path/to/existing.rdb\")"}

// Open the connection
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/existing.rdb\")",
  "key": {"atomese": "(Predicate \"*-open-*\")"},
  "value": {"atomese": "(VoidValue)"}
}

// Load all data into the AtomSpace
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/existing.rdb\")",
  "key": {"atomese": "(Predicate \"*-load-atomspace-*\")"},
  "value": {"atomese": "(VoidValue)"}
}

// Now you can work with the data...

// When done, close the connection
{
  "atomese": "(RocksStorageNode \"rocks:///path/to/existing.rdb\")",
  "key": {"atomese": "(Predicate \"*-close-*\")"},
  "value": {"atomese": "(VoidValue)"}
}
```

## Important Notes

1. **Always close connections**: Failing to close may result in data loss or
   corruption.

2. **Store atoms explicitly for Values**: When you store an Atom that appears
   inside a Link, the Link structure is saved, but the Values attached to
   that inner Atom are NOT automatically saved. To save Values, explicitly
   store the Atom that has Values attached.

3. **Fetch before modify**: When working with an existing database, fetch
   atoms before modifying them to ensure you have the latest Values.

4. **VoidValue for no-argument messages**: Messages like `*-open-*` and
   `*-close-*` require `(VoidValue)` as the value argument.

5. **Links cannot contain Values**: Remember that the value argument is
   passed separately to the MCP tool, not embedded in Atomese. You cannot
   write `(SetValue ... (SomeValue ...))` because Links cannot contain Values.

## See Also

- WorkingWithValues-Resource.md - For understanding the Atom/Value distinction
- https://wiki.opencog.org/w/StorageNode - Full API documentation

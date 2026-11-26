9P2000 Filesystem Support
=========================
This directory implements Plan 9 filesystem access to the AtomSpace:
a wire-protocol codec, a synthetic file tree backed by an AtomSpace,
and the per-connection session dispatcher that serves that tree over
9P2000. Protocol references are the Plan 9 manual pages intro(5),
version(5), walk(5), open(5), read(5), remove(5) and stat(5); see
https://9fans.github.io/plan9port/man/man9/ for the specification.

The code is layered into two libraries:

* `nine9p` -- the wire-format codec (`Nine9P.{h,cc}`): message
  framing (`size[4] type[1] tag[2] ...`), Qids, and Stat directory
  entries. Everything is encoded to and decoded from plain
  `std::string` byte buffers; there are no dependencies beyond the
  C++ standard library, and in particular no dependency on the
  AtomSpace.

* `ninefs` -- the synthetic filesystem (`NineNode.{h,cc}`,
  `AtomFs.{h,cc}`), Plan 9 style namespaces (`NineNamespace.{h,cc}`),
  and the session dispatcher (`NineSession.{h,cc}`), linking `nine9p`
  and the AtomSpace.

The cogserver listens for 9P connections on a dedicated port
(17564 by default; `cogserver -n <port>` or the `*-nine-port-*`
predicate on the CogServerNode). Each connection runs in the binary
message framing mode provided by `ServerSocket::act_as_9p()` in
opencog/network, and owns one `NineSession` (its fid table); the
node tree is shared between connections, so the same atom has the
same Qid everywhere. Once running, the AtomSpace can be browsed with
the plan9port `9p` client, or mounted with `9pfuse` or the Linux
kernel `v9fs` driver:

    sudo mount -t 9p -o trans=tcp,port=17564,version=9p2000 \
        127.0.0.1 /mnt/atomspace

The file tree
-------------
`AtomFs.h` documents the layout in detail. In brief:

    /by-type/<TYPE_NAME>/<atom>/   one directory per atom
    /by-handle/<id>/               the same directories, by id
    /new                           atom-creation control file

Each atom directory holds `name`, `type`, `to_string`, `outgoing/`,
`incoming/` and `values/` entries. The same atom is always the same
NineNode, so the `by-type`, `outgoing/` and `incoming/` entries are
genuine hard links (identical Qids) to the `by-handle` entries.

Atoms are named by their s-expression, written on one line with full
type names; `<id>` is the atom's content hash as fixed-width hex:

    $ ls /mnt/atomspace/by-type/ConceptNode
    '(ConceptNode "bar")'  '(ConceptNode "foo")'
    $ ls '/mnt/atomspace/by-type/ConceptNode/(ConceptNode "foo")/incoming'
    '(ListLink (ConceptNode "foo")(ConceptNode "bar"))'

An s-expression that cannot serve as a file name -- it contains a
slash or a control character, or is longer than 255 bytes -- is
listed under its id instead. Lookups are more liberal than listings:
wherever an atom is listed by s-expression it also answers to its
id, and to any other spelling that decodes to the same atom, so
`cd '(Concept "foo")'` works too. Looking a name up never adds
anything to the AtomSpace.

Values
------
`outgoing/` and `values/` are alike, except that the first is
positional and anonymous (`0`, `1`, ...) while the second is keyed
by name. Each value is a file named by its key and holding the value
as an s-expression:

    $ cd '/mnt/atomspace/by-type/ConceptNode/(ConceptNode "foo")/values'
    $ ls
    '(PredicateNode "*-TruthValueKey-*")'
    $ cat '(PredicateNode "*-TruthValueKey-*")'
    (FloatValue 0.5 0.25)

Unless the AtomSpace is read-only, values can be changed with
ordinary file operations:

    $ echo '(FloatValue 0.9 0.1)' > '(PredicateNode "*-TruthValueKey-*")'
    $ echo '(StringValue "hello")' > '(Predicate "greeting")'
    $ rm '(PredicateNode "greeting")'

Writing replaces the value, creating a file attaches a value under a
new key (the file name is the key's s-expression), and removing the
file deletes the value. Each write must hold exactly one complete
value, written at the start of the file. Atoms appearing in a value,
and a newly created key, are added to the AtomSpace. Changes go
through `AtomSpace::set_value()`, so changing an atom inherited from
a read-only parent frame copies it into the exported space
(copy-on-write) rather than touching the parent. Truncating a
value file (which `>` does before writing) leaves the value in
place, so a redirection whose write is rejected loses nothing.

There is no separate truth-value file: TruthValues are gone from the
AtomSpace, and a truth value is simply the `FloatValue` stored under
`(PredicateNode "*-TruthValueKey-*")`, as above.

A value that is itself an Atom or a LinkValue is shown as a file
holding its s-expression, like any other; presenting such values as
directories is an open design question.

Creating atoms
--------------
The `/new` control file creates atoms: write an Atomese
s-expression, then read back the new atom's path --

    echo '(Concept "foo")' > /mnt/atomspace/new
    cat /mnt/atomspace/new
    /by-handle/0123...cdef

The expression may carry values with it, which are attached whether
or not the atom already existed:

    echo '(Concept "foo" (alist (cons (Predicate "k") (FloatValue 1 2))))' \
        > /mnt/atomspace/new

A malformed s-expression makes the write fail (Rerror): each write
must hold exactly one complete, balanced expression, with nothing
but whitespace around it. If the
AtomSpace is read-only, `/new` and the value files report themselves
non-writable, and nothing can be changed.

Namespaces
----------
`NineNamespace` provides Plan 9 namespace composition: `mount()` and
`bind()` splice extra trees into the view, with `MREPL`, `MBEFORE`
and `MAFTER` ordering. A path with several trees mounted on it is a
union directory: lookups try each tree in order (the first match
shadows the rest) and directory reads list the name-deduplicated
union of all of them, exactly as described in bind(2). The union
applies only at the mount point itself: descending through an
ordinary name continues in the single (first-matching) tree, so an
entry shadowed at one level cannot resurface deeper down. Directory
listings resolve each child through the mount table, so an entry
with something mounted on it is listed under its own name but with
the mounted tree's Qid and permissions -- matching what a walk to
that entry returns. An already-open directory fid keeps the union
view captured when it was walked; mounts and unmounts made afterward
affect new walks only.

Each `NineSession` owns a private *copy* of the namespace it is
constructed from -- the rfork(RFNAMEG) model -- so per-connection
rearrangements never leak into other connections. A bind may also
graft a tree onto a fresh path that no underlying tree contains;
such a path is walkable but does not appear in its parent
directory's listing.

Changing the tree
-----------------
The session implements no file semantics of its own. It enforces the
protocol rules and hands every change to the node concerned, through
four hooks on `NineNode`, all of which refuse by default:

    Twrite                          write()
    Topen with OTRUNC,
      Twstat changing the length    truncate()
    Tcreate                         create()     (on the directory)
    Tremove                         remove()

A node is therefore immutable unless it overrides one of these, and
making some part of the tree changeable is a matter of overriding
them there; the session needs no change. At present `/new` accepts
`write()` and `truncate()`, a `values/` directory `create()`, and a
value file `write()`, `truncate()` and `remove()`.

Twstat can do nothing beyond that: renames and changes of mode or
ownership are refused. A Twstat that changes nothing succeeds -- in
particular the all-"don't touch" form, which asks for a sync -- and
on a writable file the times may be "set" (none are kept). This
matters on Linux, whose v9fs client truncates by Twstat: a shell
`>` redirection opens the file, sends a Twstat setting the length to
zero, and only then writes.

Errors
------
A 9P2000 Rerror carries only a string. Plan 9 shows it to the user;
the Linux v9fs client turns it into an errno by exact match against
a fixed table (`net/9p/error.c` in the kernel), and reports any
string it does not know as `Unknown error 526`. Every error this
server sends is therefore one of the `NINE_ERR_*` strings of
`Nine9P.h`, all of them entries of that table (`file not found` is
ENOENT, `permission denied` is EACCES, `read only file system` is
EROFS, `Invalid argument` is EINVAL, and so on). Nodes report
failure by throwing a `NineError` naming one of those strings; the
description of what actually went wrong goes to the cogserver log
(at debug level), not onto the wire.

Simplifications
---------------
* Only values can be changed in place. Atoms are created through
  `/new`, and cannot be removed or renamed; directories cannot be
  created.
* Each write to `/new` or to a value file must carry one complete
  s-expression, so it must fit in a single Twrite (the negotiated
  msize, 8192 bytes at most, less 23).
* A directory Tread at offset 0 snapshots the listing for that fid;
  subsequent sequential reads page through the snapshot, each reply
  carrying an integral number of stat entries, per read(5).
* Rread replies are clamped to the negotiated msize, and oversized
  Twrite payloads are rejected.
* Opening with ORCLOSE is refused, and no timestamps are kept.
* Tauth is rejected (no authentication) and Tflush is acknowledged
  trivially (dispatch is strictly synchronous).
* Namespace rearrangement is a server-side (C++) API; there is no
  in-protocol mount/bind (9P2000 itself has none -- in Plan 9 those
  are system calls on the client).

The transport is handled elsewhere: `nine_decode()` consumes exactly
one complete message from the front of a byte buffer, returning 0 if
the buffer does not yet hold a complete message, so it can be driven
either by the cogserver network layer (which delivers whole messages)
or by a simple buffered read loop.

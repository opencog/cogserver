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

    /by-type/<TYPE_NAME>/<id>/     one directory per atom
    /by-handle/<id>/               the same directories, by id
    /new                           atom-creation control file

where `<id>` is the atom's content hash as fixed-width hex, and each
atom directory holds `name`, `type`, `to_string`, `tv`, `outgoing/`,
`incoming/` and `values/` entries. The `tv` file is writable
("<strength> <confidence>"); the atom entries are otherwise
read-only. The same atom is always the same NineNode, so
`outgoing/`/`incoming/` entries are genuine hard links (identical
Qids) to the `by-handle` entries.

The `/new` control file creates atoms: write an Atomese
s-expression, then read back the new atom's path --

    echo '(Concept "foo")' > /mnt/atomspace/new
    cat /mnt/atomspace/new
    /by-handle/0123...cdef

A malformed s-expression makes the write fail (Rerror): each write
must hold exactly one complete, balanced expression, with nothing
but whitespace around it. If the
AtomSpace is read-only, `/new` reports itself non-writable.

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

Simplifications
---------------
* The tree is structurally read-only: Tcreate and Twstat are
  rejected; Tremove is refused with an error, though the fid is
  still clunked, per remove(5). New atoms are made through the
  `/new` control file instead.
* A directory Tread at offset 0 snapshots the listing for that fid;
  subsequent sequential reads page through the snapshot, each reply
  carrying an integral number of stat entries, per read(5).
* Rread replies are clamped to the negotiated msize, and oversized
  Twrite payloads are rejected.
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

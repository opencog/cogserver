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
  `AtomFs.{h,cc}`) and the session dispatcher (`NineSession.{h,cc}`),
  linking `nine9p` and the AtomSpace.

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

where `<id>` is the atom's content hash as fixed-width hex, and each
atom directory holds `name`, `type`, `to_string`, `tv`, `outgoing/`,
`incoming/` and `values/` entries. The `tv` file is writable
("<strength> <confidence>"); everything else is read-only. The same
atom is always the same NineNode, so `outgoing/`/`incoming/` entries
are genuine hard links (identical Qids) to the `by-handle` entries.

Simplifications
---------------
* The tree is structurally read-only: Tcreate and Twstat are
  rejected; Tremove merely clunks the fid, per remove(5).
* Directory reads return the whole listing at offset 0 and EOF at
  any nonzero offset, rather than supporting incremental paging.
* Tauth is rejected (no authentication) and Tflush is acknowledged
  trivially (dispatch is strictly synchronous).
* Plan 9 namespace composition (bind/mount, union directories) and
  an atom-creation control file are left for a later layer.

The transport is handled elsewhere: `nine_decode()` consumes exactly
one complete message from the front of a byte buffer, returning 0 if
the buffer does not yet hold a complete message, so it can be driven
either by the cogserver network layer (which delivers whole messages)
or by a simple buffered read loop.

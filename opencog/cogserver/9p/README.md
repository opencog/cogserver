9P2000 Wire Protocol Codec
==========================
This directory holds a minimal, from-scratch encoder/decoder for the
Plan 9 "9P2000" filesystem namespace protocol, as documented in the
Plan 9 manual pages intro(5), version(5), walk(5), open(5), read(5)
and stat(5). See https://9fans.github.io/plan9port/man/man9/ for the
protocol specification.

Only the wire format is implemented here: message framing
(`size[4] type[1] tag[2] ...`), Qids, and Stat directory entries.
Everything is encoded to and decoded from plain `std::string` byte
buffers; there are no dependencies beyond the C++ standard library,
and in particular no dependency on the AtomSpace.

The server that dispatches these messages against an AtomSpace-backed
synthetic filesystem (so that Atoms can be browsed with `9p`, or
mounted with `9pfuse` or the Linux kernel `v9fs` driver) is built in
higher layers, on top of this codec and on top of the binary message
framing provided by `ServerSocket::act_as_9p()` in opencog/network.

The transport is handled elsewhere: `nine_decode()` consumes exactly
one complete message from the front of a byte buffer, returning 0 if
the buffer does not yet hold a complete message, so it can be driven
either by the cogserver network layer (which delivers whole messages)
or by a simple buffered read loop.

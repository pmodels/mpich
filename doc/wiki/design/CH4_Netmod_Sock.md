# CH4 Sock Netmod

This page describes the design of the CH4 `sock` netmod, a minimal network
module that runs over TCP sockets. The source is in
`src/mpid/ch4/netmod/sock/`.

## Purpose

The `ofi` and `ucx` netmods both depend on an external communication
library. The `sock` netmod depends only on the operating system's socket
API, in the same spirit as the TCP netmod of CH3/Nemesis. It is meant to be:

- a fallback that works wherever TCP/IP works,
- a small reference implementation of the netmod API,
- a way to exercise the CH4 generic (MPIDIG) code paths without a network
  library.

It is not meant to compete with `ofi` or `ucx` on performance.

It is selected explicitly with `--with-device=ch4:sock`. Configure never
picks it automatically.

## Scope

The netmod implements only the *active message* (AM) part of the netmod API
declared in `src/mpid/ch4/ch4_api.txt`. Every *native* API function
(point-to-point, probe, RMA, partitioned communication) uses
the generic fallback in `src/mpid/ch4/netmod/include/netmod_am_fallback.h`,
which calls the MPIDIG implementation. MPIDIG in turn calls back into the
netmod's AM functions to move bytes.

```
MPI_Send
  └─ MPIDI_NM_mpi_isend            (netmod_am_fallback_send.h)
      └─ MPIDIG_mpi_isend          (matching, protocol; src/mpid/ch4/src)
          └─ MPIDI_NM_am_isend     (sock netmod: bytes on the wire)
```

Because the fallback headers are included unconditionally, the netmod does
not need `--enable-ch4-am-only`.

Not supported: native tagged or RMA paths, `am_tag_send` / `am_tag_recv`,
more than one VCI, and peers with a different byte order.

## Netmod API answers

| API | Behavior |
|---|---|
| `am_send_hdr`, `am_isend` | Write header and data to the peer's socket; see [Send path](#send-path) |
| `am_send_hdr_reply`, `am_isend_reply` | Same as the non-reply versions |
| `am_check_eager` | True if `am_hdr_sz + data_sz` is within the eager size |
| `am_eager_limit`, `am_hdr_max_sz`, `am_eager_buf_limit` | The eager size |
| `am_can_do_tag` | Always false |
| `am_get_data_copy_cb` | Always NULL |
| `progress` | One non-blocking `poll`; see [Progress](#progress) |
| `comm_set_vcis` | One VCI for every process |
| window, partitioned, `alloc_mem` functions | Thin wrappers that call `MPIDIG_*` |
| communicator, datatype and op hooks | Empty, except for the address exchange in the commit pre-hook |

The eager size is set by `MPIR_CVAR_CH4_SOCK_EAGER_SIZE` (default 16384
bytes). It is capped by `MPIR_CVAR_CH4_PACK_BUFFER_SIZE` (default 16384),
because `MPIDIG_am_check_init` asserts that the eager buffer limit fits in a
pack buffer. To use a larger eager size, raise both.

MPIDIG does the tag matching, so the netmod reports the default number of
tag bits.

## Message sizes

MPIDIG decides between eager and rendezvous by calling `am_check_eager`:

- **Eager:** header and data go in a single `am_isend`. If the message is
  unexpected, MPIDIG buffers it. The eager size bounds that buffering.
- **Rendezvous:** MPIDIG sends a request-to-send with `am_send_hdr`. When
  the receive is matched, the receiver replies with clear-to-send, and the
  sender then calls `am_isend_reply` once with the *entire* payload
  (handler `MPIDIG_SEND_DATA`).

So `am_isend` must accept a message of any size. A TCP stream has no message
size limit, so the netmod does not segment messages.

## Wire format

Each message on a socket is:

```
+---------------------------+
| MPIDI_SOCK_am_header_t    |   fixed size
|   handler_id   (uint16_t) |
|   src_vci      (uint8_t)  |
|   dst_vci      (uint8_t)  |
|   am_hdr_sz    (uint32_t) |
|   data_sz      (uint64_t) |
+---------------------------+
| AM header                 |   am_hdr_sz bytes, opaque to the netmod
+---------------------------+
| payload                   |   data_sz bytes, packed if non-contiguous
+---------------------------+
```

All processes are assumed to have the same data representation.

The first thing sent on a connection, before any message, is a 16-byte
*hello*: a magic number and a protocol version.

Handler ID `0xffff` is reserved for the netmod's own dynamic messages; see
[Dynamic processes](#dynamic-processes).

## Addresses

Each process opens one listening socket during `init_local`. Its address is
a fixed-size binary record, `MPIDI_SOCK_name_t`, holding a socket address
and the port. The same record serves as the *business card* exchanged at
startup and as the *upid* used by dynamic processes. Records are compared as
bytes.

| CVAR | Default | Meaning |
|---|---|---|
| `MPIR_CVAR_CH4_SOCK_IFACE` | Not set | Name of the network interface whose address is advertised. By default it is the first interface that is not the loopback, or the loopback if there is no other. |
| `MPIR_CVAR_PORT_RANGE` | `0:0` | Range of ports to listen on, as `<low>:<high>`. The default is any free port. It is shared with the ch3 tcp and sock channels, and `mpiexec` reads the same environment variable for its own port. |

The socket listens on all interfaces; the interface only selects the address
that peers connect to.

The netmod's part of an address vector entry (`MPIDI_SOCK_addr_t`) holds:

- the peer's address record,
- a pointer to the outgoing connection, NULL until the first send.

Addresses are exchanged in the communicator commit pre-hook for bootstrap
communicators, using `MPIDIU_bc_exchange_node_roots` for node roots followed
by an allgather for the rest, the same pattern as the `ucx` netmod.
Inserting an address only stores it; no socket is opened.

## Connections

Connections are *lazy*: a socket to a peer is opened on the first send to
that peer.

Connections are *one-directional*. A process sends only on sockets it
connected, and receives only on sockets it accepted. Two processes that
both send to each other therefore use two sockets.

```
   process A                         process B
   connect() ───── socket 1 ───────▶ accept()      A sends, B receives
   accept()  ◀──── socket 2 ──────── connect()     B sends, A receives
```

The connect call is blocking, and the socket is made non-blocking right
after. A TCP connect completes once the connection is in the peer's listen
queue, so it does not depend on the peer calling `accept`.

The connecting side writes the hello right after the connect and may then
send messages at once; it does not wait for a reply. The accepting side
checks the hello and quietly drops a connection that does not start with
the right one, or that closes before sending it. This keeps port scanners
and other strays from being read as messages. The accepting side does not
need to know which process the peer is, because the active message headers
carry what the upper layer needs.

Outgoing sockets have `TCP_NODELAY` set, since each message is written in
one piece.

### Alternative considered: one bidirectional socket per pair

CH3/Nemesis TCP uses one socket per pair for both directions
(`src/mpid/ch3/channels/nemesis/netmod/tcp/socksm.c`). With lazy connect,
both sides may connect at the same time (*head-to-head*), and one of the two
sockets must be discarded by a rule both sides agree on. CH3 does this with
an ID packet, a search for a competing socket, a tie-break on rank (or
process group ID across groups), an ACK or NAK reply, and migration of
queued sends from the losing socket.

| | Two one-directional sockets | One bidirectional socket |
|---|---|---|
| File descriptors per pair | 2, if both sides send | 1 |
| Handshake | One-way hello; send right after connect | ID, then wait for ACK or NAK |
| Connection states | None beyond open or closed | About eight, with close-and-retry paths |
| Sends queued during setup | Stay on their socket | Migrate if the socket loses |
| Dynamic processes | No special case | Tie-break key must work across groups |

The one-directional model was chosen for simplicity. Its cost is file
descriptors, addressed below. The send and receive paths do not depend on
the model, so it can be changed later within the connection code.

### File descriptor limit

A process may need up to two descriptors per peer it communicates with. The
common default soft limit on Linux is 1024 open files per process, with a
much higher hard limit. During `init_local` the netmod raises the soft
`RLIMIT_NOFILE` to the hard limit with `setrlimit`. If that fails, the
netmod continues with the existing limit; running out of descriptors is
reported when creating a socket or `accept` fails.

The netmod uses `poll`, not `select`, so descriptor values above 1023 are
not a problem.

## Send path

Each outgoing connection has a queue of pending sends.

1. If the queue is empty, the netmod tries to write the netmod header, the
   AM header and the data right away, with one `sendmsg` call.
2. If the write is partial or would block, the remainder is queued and
   written from `progress`.
3. When a message with a request is fully written, the netmod calls
   `MPIDIG_global.origin_cbs[handler_id](sreq)`.

Buffer ownership:

- **Contiguous data** is written directly from the user buffer. The request
  stays pending until the write completes.
- **Non-contiguous data and GPU buffers** are packed into a buffer that the
  netmod allocates for the whole message and frees once the message is
  written.
- **Headers.** The caller's AM header is only valid during the call. If the
  message is queued, the netmod header and the AM header are copied into the
  queue element.

Messages to one peer are written in the order they were issued, which gives
MPIDIG the per-peer ordering it relies on.

## Receive path

Each incoming connection has a small state machine:

```
                     ┌─▶ read AM header and payload ──────────────▶ dispatch ─┐
read netmod header ──┤   (buffered)                                           │
        ▲            └─▶ read AM header ─▶ handler ─▶ read payload ─▶ complete┤
        │                (direct)                                             │
        └─────────────────────────────────────────────────────────────────────┘
```

Any step may return with a partial read; the state is kept on the
connection and resumed at the next `progress`.

A message is handled in one of two ways, chosen after the netmod header is
read:

- **Buffered.** Used for messages up to the eager size, and for dynamic
  messages of any size. The AM header and the payload are read into a
  temporary buffer, and the netmod calls
  `MPIDIG_global.target_msg_cbs[handler_id](am_hdr, data, data_sz, attr, NULL)`.
- **Direct.** Used for larger messages. In practice these are
  `MPIDIG_SEND_DATA`, for which the receive is already matched. The netmod
  reads only the AM header, and calls the target callback with
  `MPIDIG_AM_ATTR__IS_ASYNC` and no data to obtain the receive request. If
  the receive buffer is contiguous, in host memory and large enough, the
  payload is read from the socket straight into it. Otherwise (a
  non-contiguous datatype, a GPU buffer, or a truncated receive) the payload
  is read in 64 KB chunks that are passed to `MPIDIG_recv_copy_seg`. If the
  callback returns no request, the payload is read and dropped. At the end
  the netmod calls the request's `target_cmpl_cb`.

The netmod never leaves a payload unread in a socket while waiting for a
receive to be posted. Doing so would block every later message from that
peer. This is why unexpected data is limited to the eager size and larger
messages go through rendezvous.

## Progress

`MPIDI_NM_progress(vci, made_progress)` returns immediately unless `vci` is
0. Otherwise it makes one `poll` call with a zero timeout over the listening
socket and all connections, and then:

- flushes the send queue of writable outgoing sockets,
- advances the receive state machine of readable incoming sockets,
- accepts new connections.

Outgoing sockets are polled for writability only while their send queue is
not empty. At most 16 messages are read from one connection in one call, so
that a busy connection does not starve the others.

## Threads and VCIs

The netmod uses a single VCI, VCI 0. All netmod state is protected by the
VCI 0 lock, which the fallback paths and the progress engine already hold
when they call into the netmod.

## Shared memory

The netmod works with or without the POSIX shmmod. With shared memory
enabled, an `MPI_ANY_SOURCE` receive is posted to both the shmmod and the
netmod as partner requests; the AM fallback receive path handles the
partner, as it does for other netmods in AM-only mode.

## Dynamic processes

The netmod defines `MPIDI_BUILD_CH4_UPID_HASH` and implements:

- `get_local_upids` and `insert_upid`, using the address record as the
  upid;
- `dynamic_send`, `dynamic_recv` and `dynamic_sendrecv`, used before two
  processes share a communicator: in `MPI_Intercomm_create` as well as in
  connect and accept. These messages use the reserved handler ID and are
  matched on context ID and tag. Received ones are kept in a queue inside
  the netmod. A send copies its data and returns; a receive blocks on
  netmod progress until a matching message arrives or the timeout expires.

## Finalize

`mpi_finalize_hook` flushes all send queues, then waits on a PMI barrier
(unless `MPIR_CVAR_NO_COLLECTIVE_FINALIZE` is set) so that no process closes
its sockets while others may still use them, and then closes all sockets
and frees the upid hash.

## Errors

Socket errors are found by `sendmsg`, `read`, `connect` and `accept`, and are
returned as MPI errors from the send call or from `progress`. The messages
name the peer's address. Writes use `MSG_NOSIGNAL` so that a closed peer
does not raise `SIGPIPE`.

A connection that gets an error is closed, so the error is reported once:

- **Outgoing.** The requests of the messages still in its send queue are
  completed with an error of class `MPI_ERR_OTHER`, and every later send to
  that peer fails. The netmod does not reconnect.
- **Incoming.** A peer that closes its socket between messages is normal: it
  has finalized, and the connection is closed quietly. A close in the middle
  of a message, or a read error, is reported from `progress`. Receives that
  wait for that peer are not completed; the netmod does not know which
  process an incoming connection belongs to.

## Debugging

With `MPIR_CVAR_DEBUG_SUMMARY=1`, rank 0 prints the listening address and
port, the file descriptor limit and the eager size.

## Testing on one machine

With the default configuration, processes on one node talk through shared
memory and the netmod sees no traffic. There are two ways to exercise it on
a single machine:

- Set `MPIR_CVAR_ODD_EVEN_CLIQUES=1`. Processes of odd and even rank are
  then treated as two nodes. This also covers the multi-node address
  exchange and the shared memory plus netmod `MPI_ANY_SOURCE` path.
- Configure with `--with-ch4-shmmods=none`. All traffic then goes through
  TCP.

## Limitations and possible future work

- **Blocking connect.** The first send to a peer blocks until the TCP
  connect completes.
- **Failed peers.** A process that waits to receive from a peer that died is
  not told. The hello does not carry a job or process identifier, so an
  incoming connection cannot be tied to a process.
- **Progress cost.** Every netmod progress call makes a `poll` system call,
  even when nothing is pending. With shared memory enabled, CH4 only calls
  netmod progress when the shmmod made no progress, and polling less often
  was measured to gain about 5% in shared memory ping-pong latency while
  adding to the TCP latency, so it was not done.
- **Closed connections** keep their slot in the poll array until finalize,
  so a process that connects and disconnects many times grows the array.
- **Non-contiguous sends** pack the whole message, whatever its size.
- **Dynamic processes.** `insert_upid` does not set a node ID for processes
  of another `MPI_COMM_WORLD`, so they are never treated as local.
- **Single VCI.** More VCIs would need a set of connections per VCI.

## Source layout

Under `src/mpid/ch4/netmod/sock/`:

| File | Content |
|---|---|
| `sock_pre.h` | Types embedded in CH4 objects (`MPIDI_SOCK_name_t`, `MPIDI_SOCK_addr_t`, and placeholders for the rest) |
| `sock_types.h`, `sock_impl.h`, `globals.c` | Internal types, prototypes and global state |
| `netmod_inline.h` | Includes the AM fallback and the netmod's inline headers |
| `sock_am.h` | Inline AM functions |
| `sock_progress.h`, `sock_request.h`, `sock_proc.h` | Other inline functions |
| `sock_init.c` | Init, finalize, address exchange, CVARs |
| `sock_conn.c` | Poll array, connect, hello, accept, close |
| `sock_io.c` | Send queue, receive state machine, progress |
| `sock_comm.c`, `sock_datatype.c`, `sock_op.c` | Hooks |
| `sock_win.c`, `sock_part.c` | Wrappers that call `MPIDIG_*` |
| `sock_spawn.c` | Dynamic messages and upids |
| `errnames.txt` | Error messages |
| `Makefile.mk`, `subconfigure.m4` | Build |

The netmod is registered in `maint/gen_ch4_api.py`, which generates
`func_table.c` and `sock_noinline.h` for it. See
[CH4 API Auto-Generation](../notes/ch4_api.md).

## Related pages

- [CH4 Overall Design](CH4_Overall_Design.md)
- [CH4 Process Address Translation](CH4_Process_Address_Translation.md)
- [Establishing Socket Connections](Establishing_Socket_Connections.md) and
  [Sock conn protocol](Sock_conn_protocol.md) (CH3, for comparison)

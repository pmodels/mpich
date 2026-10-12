# CH4 OFI Active Messages

This page describes how the CH4 OFI netmod sends and receives active
messages (AM): the protocols, how the send path is layered, how sends are
deferred and ordered, and how the payload is kept aligned. Read it before
changing the AM code under `src/mpid/ch4/netmod/ofi/`.

| File | Contents |
|---|---|
| `ofi_am.h` | entry points: `MPIDI_NM_am_isend`, `MPIDI_NM_am_send_hdr`, `MPIDI_NM_am_check_eager`, size limits |
| `ofi_am_impl.h` | protocol drivers `MPIDI_OFI_do_am_isend_*`, message senders `MPIDI_OFI_am_isend_*`, `MPIDI_OFI_am_init_sreq` |
| `ofi_progress.h` | reissuing deferred sends: `MPIDI_OFI_handle_deferred_ops` |
| `ofi_events.c` | receive dispatch (`am_recv_event`) and send completions |
| `ofi_am_events.h` | receive handlers for each message type |
| `ofi_pre.h`, `ofi_types.h` | wire header, request header, deferred request, size macros |

## Message types

Every AM is one or more `fi_send` messages, each laid out as:

```
| MPIDI_OFI_am_header_t (24 B) | AM header + padding (am_hdr_sz) | payload |
```

| `am_type` | When | Payload |
|---|---|---|
| `SHORT_HDR` | header only (`MPIDI_NM_am_send_hdr`, sent with `fi_inject`) | none |
| `SHORT` (eager) | the whole message fits in `MPIDI_OFI_DEFAULT_SHORT_SEND_SIZE` | all the data |
| `PIPELINE` | larger data, without RMA | one segment; only the first segment carries the AM header |
| `RDMA_READ` | larger data, with RMA | LMT info; the receiver `fi_read`s the data and acks |

`MPIDI_NM_am_isend` chooses the type by size. `MPIDI_NM_am_check_eager`
lets MPIDIG point-to-point decide ahead between sending data with the header
(`true`) and an RTS/CTS handshake (`false`). The netmod records its choice
in `am_type_choice`. MPIDIG doesn't depend on which OFI type is used: every
type delivers the data to the MPIDIG target callback without help from above
the netmod.

## Send path: drivers and senders

```
MPIDI_NM_am_isend ───────────────┐
                                 ├─> MPIDI_OFI_do_am_isend_{eager,pipeline,rdma_read}   (drivers)
MPIDI_OFI_handle_deferred_ops ───┘         └─> MPIDI_OFI_am_isend_{short,pipeline,long}   (senders)
   (progress, issue_deferred = true)                   └─> fi_send
```

- **Drivers** decide *whether and when* a message can go out. They set up the
  request (`MPIDI_OFI_am_init_sreq` copies the caller's AM header into
  `sreq_hdr->am_hdr_buf`), enforce ordering, get buffers, defer, split the
  pipeline into segments, and remove entries from the deferred queue.
- **Senders** build and post exactly one message from the arguments they are
  given. They never defer or allocate request-level resources.

## Deferral and ordering

- Each VCI has a FIFO, `deferred_am_isend_q`. Once it is non-empty, every new
  send on that VCI must queue behind it, so AM order is preserved.
- `DEFER_AM_SEND` saves everything a reissue needs in
  `MPIDI_OFI_deferred_am_isend_req_t`, including `am_hdr_sz`. The AM header
  bytes stay in `sreq_hdr->am_hdr_buf`. Progress reissues the head entry
  with those saved arguments, so a driver's arguments are valid both on the
  first call and on reissue.
- Pipeline: segments after the first always go through the queue. For those,
  progress passes `am_hdr_sz = 0`, since only the first segment carries the
  AM header.
- **Eager never defers.** Its only benefit is sending right away. If it would
  have to wait, for ordering or for a pack buffer, the eager driver hands the
  send to the pipeline driver, which handles waiting. The receiver handles a
  `PIPELINE` message of any size, so layers above don't notice.

## Buffers

| Buffer | Source | Used for | Freed |
|---|---|---|---|
| request header cell (`sreq_hdr`) | `am_hdr_buf_pool`, 1 KB | AM header copy; holds the whole message in place when it fits (`MPIDI_OFI_AM_MAX_MSG_SIZE`) | with the request |
| pack buffer | `pack_buf_pool` (`MPIR_CVAR_CH4_PACK_BUFFER_SIZE`) | eager or one pipeline segment that doesn't fit in place | on send completion |
| `send_req` | `am_hdr_buf_pool` | one pipeline segment's OFI context | on segment completion |
| RDMA-read buffer | `MPL_malloc` | packed data for the receiver to read | on the read ack |

A pipeline segment is built in place when the *rest* of the message fits,
which can only be the only segment or the last one. At most one segment per
message is built in place.

## Receive path

`am_recv_event` checks the per-source sequence number. A message that
arrives early is copied into an `MPIDI_OFI_am_unordered_msg_t` and processed
when its turn comes. Messages are then dispatched by `am_type`:
- eager: call the MPIDIG target callback;
- pipeline: call it on the first segment, cache the request by source
  (`MPIDIG_req_cache_*`), and copy each segment with
  `MPIDIG_recv_copy_seg`;
- RDMA read: read the data, then send `MPIDI_OFI_AM_RDMA_READ_ACK`.

## Payload alignment

Applies when `NEEDS_STRICT_ALIGNMENT` is defined, which is the default unless
configured with `--enable-fast=no-strict-alignment`. Non-contiguous pack and
unpack use typed loads and stores on the payload, so it must be aligned to
`MAX_ALIGNMENT`.

```
payload = message start + sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz
```

- **The sender keeps the offset aligned.** Eager and pipeline add
  `MPIDI_OFI_AM_PADDING_SZ(sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz)` to
  `am_hdr_sz` and zero the padding bytes. Later pipeline segments get just
  the padding. The padding is part of `am_hdr_sz` on the wire, so the receiver
  doesn't need to know about it. `MPIDI_OFI_MAX_AM_HDR_SIZE` (248 on x86-64)
  keeps the padded size within the 8-bit field. Messages are built only in
  aligned buffers (`msg_hdr` is `MPL_ATTR_ALIGNED`, pool cells are aligned).
- **The receiver handles the message start.** `FI_MULTI_RECV` may place a
  message at a smaller alignment. In that case the headers are copied to an
  aligned temporary buffer, and `align_payload` moves the payload down in place
  (by at most `MAX_ALIGNMENT - 1` bytes, inside the same message's header
  area). Out-of-order messages are copied to an aligned buffer and are never
  shifted.
- RDMA read (the LMT info is copied out) and header-only messages need
  nothing.
- **Cost:** up to 15 bytes of padding per eager or pipeline message, and one
  `memmove` per message the provider places misaligned.

## Invariants

- Only drivers defer or allocate request-level resources. Senders send one
  message from their arguments.
- A driver's `am_hdr_sz` is valid on every path. Anything a reissue needs is
  saved in the deferred request or in `sreq_hdr`.
- The AM header copy in `sreq_hdr` lives until the request completes.
- The receiver finds sizes and the payload only from the OFI header
  (`am_hdr_sz`, `payload_sz`).
- Under strict alignment, `sizeof(MPIDI_OFI_am_header_t) + am_hdr_sz` is a
  multiple of `MAX_ALIGNMENT` for every eager and pipeline message.
- The receiver shifts a payload only after the headers have been copied out
  of the receive buffer.

## Knobs

- `MPIR_CVAR_CH4_OFI_ENABLE_RMA`, `MPIR_CVAR_CH4_OFI_AM_LONG_FORCE_PIPELINE`:
  choose RDMA read or pipeline for large data.
- `MPIR_CVAR_CH4_PACK_BUFFER_SIZE`, `MPIR_CVAR_CH4_MAX_NUM_PACK_BUFFERS`:
  pack buffer size and pool limit. A small pool makes sends defer, which is
  useful for exercising the deferred path in tests.

# Communicators And Context IDs

## What Is A Context ID?

When MPI receives a message and matches it against MPI_Recv requests,
it compares the message's envelope to the MPI_Recv's envelope. The
envelope is the triple of (source, tag, communicator). The source and
tag are explicitly integers, yet the communicator is a logical construct
indicating a particular communication context. In MPICH this context is
implemented via an additional tag field known as the context id. It's
worth remembering that there is no wild card matching for communicators.

The MPICH context ID is a 16-bit integer field that is structured as
follows:

In this crude diagram each character represents a bit. There are three
fields of the context ID indicated by letter and color:

- If 0, this is a traditional context ID allocated from the context ID
  mask ([explained below](#context-id-mask)). If 1 this is
  a context ID used for dynamic processes and is allocated in a
  different manner.
- This is the index into the context ID mask ([explained below](#context-id-mask)).
- This is which bit index within the mask word that this ID refers to.
- This bit is set to 1 if this communicator is a local communicator
  for an intercommunicator. This allows us to derive local
  communicator context IDs from the intercommunicator context ID
  without any communication.
- This is [explained further below](#sub-communicator-type-field).
- This is used to indicate different communication contexts within a
  communicator. For example, user point-to-point messages
  (`MPI_Send/MPI_Recv`) occur in a different context than collective
  messages (`MPI_Bcast, etc`). This also [explained further below](#context-type-suffix).

The actual type of a context ID is `MPIR_Context_id_t`, which is
`typedef`ed to `uint16_t`.

All members of a communicator use the same context ID for that
communicator, but a context ID is not a globally unique ID. That is, the
communicator's group information combined with the context ID constitute
a unique ID. A great example of this is when a `MPI_Comm_split` is
called collectively each of the resulting disjoint communicators has the
same context ID yet different group information.

## Context ID Mask

The context ID mask is a bit vector that is used to keep track of which
context IDs have been allocated. In the current code it is an array of
`MPIR_MAX_CONTEXT_MASK` (64) 32-bit `unsigned int`s for a total of 2048.
Each process has its own mask and its state may vary from process to
process depending on communicator membership patterns.

### Mask Access And Multi-threading

Talk about critical sections, the local context mask, and
`lowestContextId`.

**TODO finish this section** In the mean time you can examine the code
for a better explanation:

### Problems and Gotchas

There are several issues and things to watch out for when working on the
context ID code in `commutil.c`.

- The current code expects that `unsigned int` values are 32-bits or
  larger. The comments imply that it needs *exactly* 32-bit `unsigned
  int`s but it looks like we lucked out and it should work with larger
  sizes as well. This needs to be cleaned up in the current code.
- IDs are allocated from the lowest available mask integer index but
  the highest available bit index within that integer. This leads to a
  nice looking pattern when the mask is viewed as a hex string via the
  `MPIR_ContextMaskToStr` function but a strange ordering of ID values
  (124, 120, 116, ..., 0, 252, 248, ..., 128, 380, etc).
- While new IDs are allocated in the fashion described just above, the
  three default communicators (`MPI_COMM_WORLD`, `MPI_COMM_SELF`, and
  `MPIR_ICOMM_WORLD`) take up bits 0-2 of word 0 (prefixes 0, 8, and
  16). In contrast, the first context ID allocated after `MPI_Init`
  will be bit 31 of word 0 (id prefix 124). This works out OK, it's
  just surprising when you are debugging and get
  `"03fffff8ffffffff..."` when you print out the mask field. It
  wouldn't hurt to change this to something less surprising if we get
  the time.
- When allocating context IDs for intercommunicators it is important
  to allocate the `recvcontext_id` and the `context_id` fields
  correctly. By convention the `recvcontext_id` is allocated from the
  local group's pool while the `context_id` is allocated from the
  remote group's pool. It is important not to reverse this, since
  `MPIR_Free_contextid` only frees the `recvcontext_id`. A reversal of
  these two fields will result in freeing unallocated IDs and leaking
  other context IDs in some cases.
- It is safe to call `MPIR_Free_contextid` on "derived" context IDs
  (such as in localcomm IDs or topological sub-communicators), however
  those IDs will not truly be freed at that point. Since they share an
  entry in the context ID bit vector with the parent context ID they
  will be ignored. The assumption is that any child IDs will be freed
  first and then the parent IDs will be freed next, at which point the
  actual context ID will be released.

## Sub-communicator Type Field

This is used to distinguish a "top-level" or "parent" communicator from
any "sub-communicators" or "child communicators" it may have. Currently
whenever a communicator is created we also create two child
communicators for it: an internode comm and an intranode comm. These
comms are used to implement SMP-aware collective operations. Rather than
calling `MPIR_Allreduce` three times where we previously called it once,
we derive context IDs for the sub-communicators from the parent
communicator's context ID. A field value of 0 indicates a parent comm, 1
indicates an intranode communicator, and 2 indicates an internode
communicator. See here for the relevant constants: .

## Context Type Suffix

The last bit of the ID is used to indicate different communication
contexts within a communicator. Point-to-point and collective
communication occur in separate contexts and use a different suffix to
form different context IDs. Theoretically File or Win communication
could use a separate context suffix value, but they don't. Because they
don't, we only use 1 bit now instead of 2 as we did previously. The
suffix values are named and can be found here: .

These values are passed as the `context_offset` argument to `MPID_Send`
and friends where it is added to the appropriate context ID stored in
the communicator structure. This is the value that is actually used for
matching.

The `MPIC_*` family of functions are convenience functions that put the
appropriate collective offset to the corresponding `MPID_*` function. In
practice this always amounts passing "1", but it leaves us the option
for more flexible changes in the future.

## Context ID API

in :

```
static char MPIR_ContextMaskToStr(void)
```

Useful to dump the state of the context mask.

```
static void MPIR_Init_contextid(void)
```

Sets all of the bits of the context mask to 1 except for bits 0,1, and 2
of word 0.

```
static int MPIR_Locate_context_bit(uint32_t local_mask[])
```

Finds the highest bit of the lowest word that is set in the given mask
and returns the corresponding context ID.

```
static int MPIR_Allocate_context_bit(uint32_t mask[], MPIR_Context_id_t id)
```

Clears the bit in `mask` corresponding to the given context `id`.

```
static int MPIR_Find_and_allocate_context_id(uint32_t local_mask[])
```

Finds the highest bit of the lowest word that is set in the given mask.
It resets that bit **in the `context_mask`** and returns the found ID
prefix.

```
int MPIR_Get_contextid(MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id)
```

Allocates a new context ID prefix collectively over the given
communicator `comm_ptr`. Returns the new context ID in `context_id`. The
core of the algorithm copies the current state of the mask to a local
buffer and then performs an `NMPI_Allreduce` with an `MPI_BAND`
operation to find the intersection of valid context IDs among all
participating processes. The result of this reduction is fed to
`MPIR_Find_and_allocate_context_id` to determine the new context ID
prefix.

```
int MPIR_Get_intercomm_contextid( MPID_Comm *comm_ptr, MPIR_Context_id_t *context_id, MPIR_Context_id_t *recvcontext_id)
```

Called by `MPIR_Comm_copy` to get context IDs for a new
intercommunicator from an old intercommunicator. Note that it returns a
pair of IDs, one for sending and one for receiving.

## When and How Context IDs Are Selected For Communicators

### Predefined Communicators

There are three predefined communicators that reserve context IDs at
`MPI_Init` time:

- `MPI_COMM_WORLD` - id prefix 0
- `MPI_COMM_SELF` - id prefix 4
- `MPI_ICOMM_WORLD` - id prefix 8

This occurs here in the code:

### Intracommunicators

#### MPI_Comm_create

Just call `MPIR_Get_contextid(old_comm_ptr)`.

#### MPI_Comm_split

Call `MPIR_Get_contextid(comm_ptr)`. This ID is the same across all the
disjoint communicators that are created. That is, if `MPI_Comm_split` is
called such that three new communicators are created, the context ID
will be the same in all three communicators (although the groups will
obviously be different between communicators).

#### MPI_Comm_dup

This calls `MPIR_Comm_copy` which in turn calls `MPIR_Get_contextid`
over the source communicator. This new context ID is used for the
duplicate communicator.

### Intercommunicators

#### MPI_Comm_create

Call `MPIR_Get_contextid(old_comm_ptr)` and use that as the
`recvcontext_id`. Rank 0s of each group exchange their allocated context
IDs and use the received values as the `context_id` field value.

#### MPI_Comm_split

Call `MPIR_Get_contextid(local_comm_ptr)` to get the `recvcontext_id`
for the new communicator. Then roots of the groups exchange context IDs
and then broadcast them to the rest of their local groups. This received
value serves as the `context_id` for the new communicator.

All communicators that result from a single collective split call have
the same context IDs (but obviously different groups).

#### MPI_Comm_dup

This calls `MPIR_Comm_copy` which in turn calls
`MPIR_Get_intercomm_contextid`. Each group generates a `recvcontext_id`
via `MPIR_Get_contextid`. Then the roots exchange that value with each
other and broadcast the result to the local group. The value received
from the other side becomes the sending context ID (the field named
`context_id` in the `MPID_Comm` structure).

#### MPI_Comm_connect

need Pavan to explain the new scheme here.

All `connect`ing processes:

- Allocate a context ID via `MPIR_Get_contextid(comm_ptr)` over the
  connecting communicator. This is the `recvcontext_id` for the new
  intercommunicator.

Then in the root:

- Connect to the port and create a temporary communicator (context ID
  4095 (Why?)) to the root on the other side from this connection.
- Exchange global process group size, local communicator size and the
  context ID determined locally. This is sent via the temporary
  communicator.

All:

- broadcast the received info on the local communicator

Just the root:

- exchange PG info with the accept side root

All:

- store the received context ID as the `context_id` for the new
  intercommunicator.

Just the root:

- synchronize with the remote root
- free the temporary communicator

All:

- barrier over the local communicator

#### MPI_Comm_accept

The counterpart to the `connect` algorithm above. It is essentially the
same except the first step is to accept the connection instead of to
initiate it.

#### MPI_Comm_spawn

This is simply implemented via a `PMI_Spawn_multiple` followed by a
`MPIR_Comm_connect/MPIR_Comm_accept` under the hood.


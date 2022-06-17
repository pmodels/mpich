# Ch3 Channels

## Overview

The CH3 device is an example implementation of the MPICH ADI3 (Abstract
Device Interface) that provides an implementation of the ADI3 using a
relatively small number of functions. These implement communication
*channels*. A channel provides routines to send data between two MPI
processes and to make progress on communications. A channel may define
additional capabilities (optional features) that the CH3 device may use
to provide better performance or additional functionality.

Different channels may be selected at build time. At this time there are
two supported channels:

- `Nemesis` - This is the default channel; it supports multiple
  communication methods and is reasonably fast
- `Sock` - This is a simple channel based on standard Unix sockets.

Channels previously available but no longer supported include

- `shm` - A shared-memory only channel
- `ssm` - Sockets and shared memory only
- `dllchan` - A channel that allows other channels to be dynamically
  loaded at run time. Only one channel could be used in any run
- `sctp` - Contributed channel that used SCTP sockets

## Basic Channel Interface

The basic channel interface consists of *definitions* and *routines*

### Routines

These are the basic CH3 routines. Documentation pages for each of these
can be generated from the file ch3/include/mpidimpl.h . There are five
groups of routines: Initialization of the channel, initialization of a
virtual connection for a channel, channel operations needed in the
creation of new process groups (for `MPI_Comm_spawn`,
`MPI_Comm_spawn_multiple`, and `MPI_Join`), Sending data, and making
progress on communication.

- `MPIDI_CH3_Pre_init` - Allows the channel to initialize before
  `PMI_init` is called, and allows the channel to optionally set the
  `rank`, `size`, and whether this process has a parent.
- `MPIDI_CH3_Init` - Initialize the channel implementation.
- `MPIDI_CH3_InitCompleted` - Perform any channel-specific
  initialization actions after `MPID_Init` but before `MPI_Init` (or
  `MPI_Initthread`) returns
- `MPIDI_CH3_Finalize` - Shutdown the channel implementation.

-----

- `MPIDI_CH3_VC_Init` - Perform channel-specific initialization of a VC
- `MPIDI_CH3_VC_Destroy` - Perform channel-specific operations when
  freeing a VC
- `MPIDI_CH3_VC_GetStateString` - Return a string describing the
  state of a VC (used for debugging and error reporting)
- `MPIDI_CH3_Get_business_card` - Return name used to establish
  connections to the calling process
- `MPIDI_CH3_Connection_terminate` - terminate the underlying
  connection associated with the specified VC

-----

- `MPIDI_CH3_PortFnsInit` - Give the channel the opportunity to
  replace the default port functions with channel-specific ones.
- `MPIDI_CH3_Connect_to_root` - Used to connect two communicators,
  using an MPI Portname
- `MPIDI_CH3_PG_Init` - Perform any channel-specific actions when
  initializing a new process group
- `MPIDI_CH3_PG_Destroy` - Perform any channel-specific actions when
  freeing a process group

-----

- `MPIDI_CH3_iStartMsg` - A non-blocking request to send a CH3 packet.
  A request object is allocated only if the send could not be
  completed immediately.
- `MPIDI_CH3_iStartMsgv` - A non-blocking request to send a CH3 packet
  and associated data. A request object is allocated only if the send
  could not be completed immediately.
- `MPIDI_CH3_iSend` - A non-blocking request to send a CH3 packet
  using an existing request object. When the send is complete the
  channel implementation will call the `OnDataAvail` routine in the
  request, if any (if not, the channel implementation will mark the
  request as complete).
- `MPIDI_CH3_iSendv` - A non-blocking request to send a CH3 packet and
  associated data using an existing request object. When the send is
  complete the channel implementation will call the `OnDataAvail`
  routine in the request, if any.

-----

- `MPIDI_CH3_Progress_start` -
- `MPIDI_CH3_Progress_end` -
- `MPIDI_CH3_Progress_poke` -
- `MPIDI_CH3_Progress_test` -
- `MPIDI_CH3_Progress_wait` -

### Definitions

  - `MPID_USE_SEQUENCE_NUMBERS` - 
  - `MPIDI_CH3_PKT_ENUM` - Packet types defined by the channel
  - `MPIDI_CH3_VC_DECL` - Additional declarations for a VC
  - `MPIDI_CH3_REQUEST_DECL` - Additional declarations for a Request
  - `MPIDI_CH3_PROGRESS_STATE_DECL` - Declarations for progress state

## Optional Channel Features

### Shared Memory Allocation

CH3 or channels can provide capability of shared memory allocation so
that other parts of MPICH can use it if possible. (For example, RMA
operations or specialized implementations of collectives that didn't
move the data through the usual communication channel.) To achieve this,
there are two options. One is to provide shared memory allocation in
CH3, another is to define an interface in CH3 and allow any channels to
provide shared memory.

Currently what we do is the second option, only Nemesis channel provide
shared memory allocation. For the first choice, one problem needed to be
solved is that Sock channel does not support shared-memory
communication. Possible solutions are as follows:

1. Shared memory allocation is optional. Sock then says "not
available".

2. Shared memory allocation is available, but not used for
communication. Sock then provides this using a resurrected version of
the shared memory allocation code or (better) the code in nemesis is
promoted to a utility. What "not used for communication" means is that
the channel progress engine does not provide any shared-memory
communication - in the case of sock, only progress on sockets is
provided.

3. Shared memory allocation is available and may be used for
communication. Sock does not do this.

CH3 must not \*require\* shared memory, but should \*accommodate\*
shared memory.

## Questions and Answers

Questions about the design, including the rationale behind the
decisions, are in this section.

1.  Q: Why use macros for the declarations of channel-specific fields in
    the request, VC, etc., rather than provide a pointer that the
    channel can use to hang its data off of?
    A: Using a pointer is a reasonable solution except when every cycle
    counts - which is the case here. At least for the data needed to
    when sending small amounts of data, eliminating all unnecessary
    memory movement or computation is critical.

-----

TODO: port from
<http://www.mpich.org/olddeveloper/design/ch32channel.htm> and
<http://www.mpich.org/olddeveloper/design/ch3parms.htm>

Note the documents listed above are very out of date. A lot has changed
in ch3 and the channel interface since these were written. It's been
said that no documentation is better than bad documentation, so read the
above at your own risk.

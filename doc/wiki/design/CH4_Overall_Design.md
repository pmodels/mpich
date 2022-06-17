# CH4 Overall Design

## Shortcomings of CH3

MPICH has relied on the CH3 device as the primary communication device
all through the "MPICH2" series and a part of the "MPICH-3.x" release
series. Unfortunately, over time, the device has accumulated a number of
hacks to accommodate newer communication models and network
architectures, much further than what it was originally designed to do.
Some of the shortcomings of the CH3 design are listed here:

- **VC model**: CH3 relies on communication in the context of "virtual
  connections" (VCs), where each peer process has a VC associated with
  it. This architecture matched networks that relied on a
  connection-oriented protocol, where VCs were a convenient way to
  keep track of the connection state and other peer-related
  information. Over time, VCs have accumulated additional fields, not
  all of which are useful to the same degree. Some of these can be
  cleaned up to reduce the size of each VC. Also, there has been some
  effort to make the allocation of VCs more dynamic to only create VCs
  to the processes we are communicating with. However, none of these
  approaches solve the fundamental scalability limitation of the VC
  structures, which scale with the number of peer processes.

- **Active-message based Communication Model**: In the CH3 device,
  almost all communication is performed in the context of active
  messages. Each communication type has a "packet type" associated
  with it, and a packet handler associated with each packet type.
  Communication relies on attaching the packet handler for each packet
  (which is fundamentally noncontiguous since the packet header is at
  a separate location than the user data), and the receiver process
  invoking a software handler to process the message. In newer MPICH
  versions, CH3 was modified to support networks that natively support
  the MPI matching model required for send/recv communication, but it
  is a (rather clumsy) workaround from the current active-message
  model. Direct support for newer matching-based networks is not
  present. A model where networks can directly handle MPI-level
  send/recv communication, through appropriate matching at the network
  layer, is required.

- **Function pointers**: The CH3 device, over many years, moved to a
  model of function pointers, where lower layers can override
  communication functionality from upper layers. However, function
  pointers are hard for current compilers to inline (even with IPO),
  making this architecture prone to high instruction counts. A model
  where inter-layer interfaces are macros or at the very least
  functions (where good compilers can inline) is required. This loses
  some of the functionality of, for example, runtime selection of
  network modules, but such functionality can be reacquired through
  multiple MPICH builds that are ABI compatible.

- **Shared-memory Functionality**: Shared memory is central to the
  design of the nemesis channel. Communication is always overridden
  for shared memory communication before being handed off to the
  network. For "network layers" that support shared memory capability
  as well, this is an unnecessary branch. For CH3, this is an
  unnecessary round-trip to the nemesis layer, to get shared memory
  support (e.g., for RMA shared memory communication). One of the
  reasons for this design in CH3 is the presence of the multiple
  channels, and the requirement in CH3 to not assume channel-specific
  features.

- **Netmod API**: CH3:nemesis hides a lot of communication
  functionality from the network layer and only hands down simple
  functions (e.g., start message) to the network. This loses semantic
  functionality forcing the network to follow a predefined
  communication model, irrespective of what functionality the network
  hardware provides.

## Design Goals for CH4

CH4 needs to be designed around the following goals:

- **Non-scalable data structures should be restricted to non-scalable
  networks**: Data structures such as VCs, that are convenient for
  connection-oriented networks, should be restricted only to
  connection-oriented networks. They should not be an integral part of
  CH4.

- **Full communication semantics provided to networks**: The device
  design should directly hand down full communication data structures
  to the network layer (e.g., communication rank, communicator,
  datatype handle), which can internally use network-specific features
  to implement it. CH4 can provide a fall-back implementation for
  networks that do not want to implement the full communication
  functionality, but such an implementation must not be forced up on
  all networks. This would also move away from the forced
  active-message model and networks are free to implement
  communication in any way they see fit.

- **Shared Memory**: CH4 should have closely integrated shared memory
  functionality (for performance). However, such capability should not
  be forced on to networks. Networks should be free to implement their
  own shared memory functionality as needed.

## CH4 Data Structures

CH4 relies on the following primary data structures:

- Process Groups (PGs): Each PG represents a **COMM_WORLD** in a
  connected group of processes. Each PG has information related to the
  size of the PG, and perhaps the offset to the start **lpid** (rank
  in the connected universe), etc.

- Communicators: Apart from information specific to each communicator,
  probably the most unscalable part of the communicator structure
  would be the mapping of the rank to the **lpid** (i.e., global
  rank). For **COMM_WORLD** and dups of **COMM_WORLD**, the rank and
  lpid are equal. For other communicators, it might not be. CH4 would
  have two models for maintaining this mapping. The first is a
  **direct** model, where a mapping array is used: constant lookup
  time but unscalable. The second is an **indirect** model, where a
  communicator can either be **simple** (world or dup of world),
  **displaced** (lpid is at a constant displacement), or **irregular**
  (requires an unscalable mapping array). Wherever mapping arrays are
  used, the mapping arrays should be maintained for all dups of a
  communicator and reference counted.

## CH4 Components

**Memory Objects** - The memory objects component is for objects such as
requests, RMA active-message ops, RMA active-message targets, and
packing buffers. Design principles include:

- Each object has a min count (allocated at init time) and a max count (dynamic allocation in chunks up to this number)
- Has the ability to do garbage collection in three modes: simple, aggressive nonblocking and aggressive blocking.  
  Different components might register hooks for these cleanups.  Simple cleans up resources without losing performance, 
  aggressive nonblocking cleans up resources but might have to fall back to a slow path, and aggressive blocking cleans 
  up resources by potentially blocking for progress.
- Some objects might have a reference count and completion count associated with them.

**Netmod API** - The following API is mandatory:


- `MPIDI_CH4I_nm_am` - CH3-like active-message functionality.  The CH4 layers gives an IOV of headers, a pointer to the user
  data (including a datatype handle), and an offset (for streaming functionality). Each message has a packet handler 
  associated with it, that is triggered on the target.  The netmod also specifies ordering semantics for these active 
  messages (e.g., read-after-write ordering).


The following API calls are optional:

- `MPIDI_CH4I_nm_isend` - MPI-like send functionality.  Takes MPI-level function arguments (except for small differences, e.g., 
  `MPID_Comm` instead of `MPI_Comm`) and returns a request.
- `MPIDI_CH4I_nm_irecv` - MPI-like receive functionality.  Handles matching logic.  Matching data structures are maintained at the CH4 layer.

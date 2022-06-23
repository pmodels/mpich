# CH3 Object Model

This page exists to clarify the relationship between different objects
(structures and the like) in MPICH2. **Please update this page if you
see something incorrect or otherwise lacking.**

We just found an 
[old design doc](Process_Groups_and_Virtual_Connections.md)
that explains some of this in additional detail. Take it with a grain
of salt, because it was probably not updated along with the code as
time went on.

## Object types

  - `VC` - Virtual Connection
  - `PG` - Process Group
  - `VCRT` - Virtual Connection Reference Table
  - `comm` - Communicator
  - `CH` - the channel-specific portion of the VC

### `sock` Specific

  - `conn` - connection, this is a proxy for the underlying tcp stream

## Object relationships

There is exactly one PG per `MPI_COMM_WORLD`. That is, in a typical
MPI program, there is only a single `PG`. In the dynamic process case
(such as `MPI_Comm_spawn`) there will be one for each world.

A `VC` represents a logical communication path between a pair of MPI
processes. One endpoint is specified implicitly by which `PG`'s
`comm_world` "owns" the `VC` and the rank of the current process, while the
other endpoint is a (`PG,rank`) pair.

A `VCRT` holds pointers to various `VC`s. Each communicator has a `VCRT`.

TODO: Flesh this out more

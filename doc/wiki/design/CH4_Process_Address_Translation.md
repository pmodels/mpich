# CH4 Process Address Translation

## Motivation

MPI processes are logically represented as integer *ranks* within
communicators, while MPI implementation internally maintains the
physical network addresses of different processes. When a MPI program
moves data between two processes by addressing them using ranks, the MPI
implementation needs to translate these ranks into the their
corresponding network address before the communication can take place.
In current MPICH, the management of network address relies on two data
structures:

1.  **virtual connections (VCs)** which contain network physical
    addresses and their associated data structures
2.  **virtual connection reference tables (VCRTs)** which contain the
    mapping between logical ranks to the VCs of the corresponding
    processes

### High Memory Usage of VC and VCRT

As each processes need to maintain the network address for its
peer-process, the VCs in each process consumes `O(P)` memory, where
`P` is the number of processes. As MPICH evolves, many data
structures are packed in the VC object for better cache locality. The
size of a typical VC object is 480 bytes. VC alone can consume 360 MB
memory in each process when running at full scale of Mira supercomputer
(768K processes).

Similarly, each VCRT requires `O(P)` memory where ``P` is the
number of ranks in the communicator. Given the fact that each
communicator can order the ranks in arbitrary order, `n`
communicators will results in `O(nP)` memory usage.

### Regular Process Mapping Models

In practical, most MPI application follow regular patterns when creating
communicators. For example, `MPI_Comm_dup` creates a child `COMM` with
identical ranks in same order as the parent `COMM`. In this case, it is
unnecessary to create a duplicated VCRT for the child `COMM`. Instead,
MPICH will refer to the VCRT of the parent `COMM`. Some other application
may lay processes in a virtual two-dimensional grid and split the
processes into *row* `COMM` and *column* COMM. The row `COMM`'s rank mapping
would be a subset of ranks in the parent `COMM` at a fixed `offset`. The
column `COMM`'s rank mapping would be a simple `strided` lookup based on
the parent communicator’s rank mapping.

## Design

### Address Vector Table

### Rank Map

## Performance Impact

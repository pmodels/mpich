# Debugger Message Queue Access

## Communicators

The model that the debugger interface uses is organized around (virtual)
separate message queues for each communicator. The debugger interface
requires a list of active communicators. Since communicator construction
is a relatively heavyweight operation, this list is maintained whether
or not the debugger support is enabled.

The structure `MPIR_Comm_list` contains a pointer to the head of the
list, along with a sequence number. The sequence number is incremented
any time a change is made to the list; this allows the debugger to
detect when it needs to rebuild its internal representation of the
active communicators.

The list of all communicators is maintained by two routines in
`src/mpi/comm/commutil.c`: `MPIR_Comm_create` (creates a new
communicator) and `MPIR_Comm_release` (frees a communicator if the
reference count is zero). The structure that contains this list is
defined in this file and the variable name is `MPIR_All_communicators`.
This is a global variable to allow the debugger to easily find it.

These routines in turn call two macros,

```
MPIR_COMML_FORGET
MPIR_COMML_REMEMBER
```

that are defined in `mpiimpl.h` to call routines whose implementations
are in `src/mpi/debugger/dbg_init.c`. These routines are

```
void MPIR_CommL_remember( MPID_Comm * );
void MPIR_CommL_forget( MPID_Comm * );
```

## Receive Queues

The receive queues are part of the device implementation; in the CH3
device, they are implemented in the file
`src/mpid/ch3/src/ch3u_recvq.c`. Normally, this file does not export the
queues directly (the head and tail pointers are `static`). To support
message queue debugging, the variables `MPID_Recvq_posted_head_ptr` and
`MPID_Recvq_unexpected_head_ptr` are exported; these are *pointers* to
the variables that hold the heads of those two lists.

## Send Queues

The definition of the "send queue" is somewhat vague. One definition is
that it is the queue of user-created `MPI_Request`s that have not been
completed with an MPI completion call such as `MPI_Wait`. Other
definitions could include any pending send operations in the
communication layer; this could include pending blocking sends in a
multi-threaded application.

This is done by adding two functions (really macros, so that they can be
easily made into no-ops when debugger support isn't included) that add
and remove user send requests from a separate list of requests.

The strategy is to add send requests to the special send queue when they
are created within the nonblocking MPI send routines (e.g.,
`MPI_Isend`). Requests are removed when the `MPIR_Request_complete`
routine is called (this routine is in `src/mpi/pt2pt/mpir_request.c`).
The list of send requests is maintained by the routines
`MPIR_Sendq_remember` and `MPIR_Sendq_forget` which are defined in
`src/mpi/debugger/dbginit.c`.

## Testing

To both test the interface and to provide a example of the sorts of
operations performed by a debugger that makes use of this interface, the
file `src/mpi/debugger/tvtest.c` (along with the support file
`src/mpi/debugger/dbgstub.c`) provides a simple example of using these
routines to access and display the message queues.

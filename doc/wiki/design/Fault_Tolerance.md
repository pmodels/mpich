# Fault Tolerance

The work done to allow fault tolerance was started by Darius Buntinas
and continued by Wesley Bland.

## Usage

In order to use the fault tolerance features of MPICH, users should not
disable this flag at configure time:

```
--enable-error-checking=all
```

This will allow the MPICH implementation to correctly check the status
of communicators when calling MPI operations.

Users will also need to enable a runtime flag for the Hydra process
manager:

```
--disable-auto-cleanup
```

This will prevent the process manager from automatically killing all
processes when any process exits abnormally.

Within the application, the most basic code required to take advantage
of any fault tolerance features is to change the error handler of the
user's communicators to at least `MPI_ERRORS_RETURN`. At the moment,
fault tolerance is only implemented for the `ch3:tcp` device. The
other devices will require some changes in order to correctly return
errors up through the stack.

## Implementation

### Error Reporting

#### Hydra

Local failures are detected by Hydra, the process manager via the usual
Unix methods (closed local socket). If a process terminates abnormally,
it is detected by the process manager in and `SIGUSR1` is used to
notify the MPI application of the failure. This notification is also
sent to the PMI server to be broadcast to all other processes so they
can also raise `SIGUSR1`. This happens in `pmi_cb` in
`src/pm/hydra/pm/pmiserv/pmip_cb.c`.

### CH3 Cleanup

When a failure is detected and the signal `SIGUSR1` is raised, it is
caught by the CH3 progress engine. This is done by comparing the number
of known `SIGUSR1` signals to the current number. If it has changed,
the progress engine calls into the cleanup functions. The first function
is `MPIDI_CH3U_Check_for_failed_procs`. In this function, CH3
gets the most recent list of failed processes from the PMI server.
Currently, value is replicated on all PMI instances, but in the future,
it could do something more scalable and do a reduction to generate this
value with local knowledge.

If some new failures are detected, they are handled by a series of
calls, first `MPIDI_CH3I_Comm_handle_failed_procs`. This function
loops over each communicator to determine if it contains a failed
process. If it does, it is marked as not *anysource_enabled*. This
means that future recv (or non-blocking receive) calls will probably
return with a failure indicating that they could not complete. However,
they will be given one chance through the progress engine to match so
it's possible that they might still finish. Also during that function,
the progress engine will be signalled to indicate that a request has
completed. This will allow the progress engine to return control back to
the MPI level where a decision can be made about whether to return an
error or return to the progress engine to continue waiting for
completion.

After cleaning up the communicator objects, the next function is
`terminate_failed_VCs`. This function closes all of the VC
connections for failed processes. This will also clean out the nemesis
send queue to remove all messages to the failed processes. It also calls
the appropriate functions to clean out both the posted and unexpected
receive queues.

Deep in the call stack, the function
`MPIDI_CH3U_Complete_posted_with_error` is called which cleans up
the posted receive queue. This is part of the process of cleaning up
failed VCs so only requests which match the failed VCs are removed (when
one is removed, it's printed to the log to make it easy to track).

#### Netmod

As mentioned previously, TCP is currently the only netmod that supports
fault tolerance. It is done by detecting that a socket is closed
unexpectedly. When that happens, the netmod calls the tcp cleanup
function (`MPID_nem_tcp_cleanup_on_error`) and returns an error
(`MPIX_ERR_PROC_FAILED`) via the usual MPICH error handling
methods.

### API

#### FAILURE_ACK / FAILURE_GET_ACKED

The failure acknowledgement functions both end up implemented in the
same file in CH3: `src/mpid/ch3/src/mpid_comm_failure_ack.c`. They
are pretty straightforward. The reason that these functions work
correctly is because of the way PMI works. If that mechanism changes,
these functions will have to be updated.

PMI always returns a string containing a list of failed processes when
asked. This string is only appended, never deleted or re-ordered.
Because of this, we can just keep track of the last failed process which
we've acknowledged (when we acknowledge a failure) and use that as the
basis to generate the group of failed processes (when we ask for the
acknowledged failures.

The value of the last acknowledged failure is stored in
`MPIDI_CH3I_comm` as `last_ack_rank`. This value is accessed via
`comm_ptr.dev-\>anysource_enabled`. In the acknowledgement function,
this value is set based on the last known failed process
(`MPIDI_last_known_failed`) which is set in
`MPIDI_CH3U_Check_for_failed_procs`.

At the same time that we set the last acknowledged failed process, we
also set a flag indicating that it's now safe to process receive
operations involving `MPI_ANY_SOURCE` by setting the flag
`anysource_enabled` in the `MPIDI_CH3I_com` object.

The function to get the group of failed process works in much the same
way. Internally, it uses `MPIDI_CH3U_Get_failed_group` to parse
the string of failed processes from PMI. When it reaches the last
acknowledged failure, it quits processing the string and creates a
group, which is returned to the user. That group is intersected with the
group of the communicator itself and returned to the user.

#### SHRINK

This function is a series of of calls to generate the group of failed
processes, determine whether the group is consistent for all ranks, and
create a new communicator from the failed processes. The group of failed
processes is generated with the call to
`MPID_Comm_get_all_failed_procs` with data from PMI. The inverse
of that group is then used with `MPIR_Comm_create_group` to attempt
to create a new communicator containing only the processes that are
alive. After the new communicator is created, the processes do an
`MPIR_Allreduce_group` with the `errflag` value to ensure that
everyone returned correctly. If any of those parts fail, the process is
started again from the beginning up to 5 times. If the operation still
can't complete after 5 attempts, the machine should be thrown away and
an error is returned.

#### AGREE

The agreement function is similar to the shrink function. However, it
also determines if any unacknowledged failures exist. It starts by
getting the acknowledged failed group from
`MPID_Comm_failure_get_acked` and then calls
`MPID_Comm_get_all_failed_procs` to get the entire list of failed
processes. These groups are compared with `MPIR_Group_compare_impl`
and if they are not equal, a flag is marked. Because failure knowledge
is local, this flag will not initially be equivalent on all processes,
meaning the collective portion of the algorithm must still be completed.
This is done with two calls to `MPIR_Allreduce_group`. The first
collectively decides the return value of the success flag, the second
does a bitwise and (`MPI_BAND`) over the user supplied integer value.
If the allreduce fails, then a failure is known at all processes and
they will all return an error collectively. Otherwise, they will return
`MPI_SUCCESS` with the new flag value included.

#### REVOKE

Revoke is the most complex function. It is implemented primarily in
`MPID_Comm_revoke`. This function has two major code paths depending
on whether the communicator has already been revoked or not.

If it has not yet been revoked, it will internally mark the communicator
as revoked with the `MPID_Comm.revoked` flag. It will do the same for
the `node_comm` and the `node_roots_comm`. It will also mark a
counter in the device portion of the communicator to track how many
other processes need to be informed about the revoke (more accurately,
how many other processes have sent a message saying that they have
revoked). Next, the function grabs a reference count of the communicator
to prevent it from being destroyed if the user does something like
revoke and immediately free the communicator. Then a message is sent to
all other processes to inform them of the revoke. If that message fails,
we decrement the counter and ignore the failure. Then we clean the
receive queues (unexpected and posted) using
`MPIDI_CH3U_Clean_recvq` to remove any messages by marking them as
`MPIX_ERR_REVOKED`.

If the communicator had already been revoked before calling (as will
happen when a message arrives to trigger the revoke message handler),
the only part that happens is to decrement the `waiting_for_revoke`
counter and release the reference to the communicator if necessary.

When the revoke message arrives at a process, it triggers a packet
handler to avoid going through the message queue. The handler
(`MPIDI_CH3_PktHandler_Revoke`) simply finds the appropriate
communicator and calls `MPID_Comm_revoke`.

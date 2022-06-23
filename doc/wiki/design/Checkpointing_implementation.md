# Checkpointing Implementation

This document describes the implementation of checkpointing in Nemesis.
For now this consists of two emails I sent describing it. Eventually
I'll clean this up.

## Email 1

BLCR allows you to checkpoint a process and all of its children. When it
does this any open sockets or mmapped files between them are preserved
after restart.

Each process registers a checkpoint callback function which is called
when a checkpoint of that process is requested. We selected the option
to have BLCR create a thread and call the callback from within that
thread.

When the app is started using hydra, it starts a proxy process on each
node. The proxy then forks the mpi processes, so the mpi processes are
children of the proxy process.

Checkpoints are initiated by hydra.

So, when a checkpoint is taken, each proxy requests a checkpoint to be
taken for itself and all of its children. BLCR then calls the callback
in each process.

Because we don't want the hydra proxy to be part of the checkpoint
image, from inside the callback, the proxy specifies that it should be
omitted from the checkpoint.

When the callback is called in an mpi process, we set a flag indicating
that the checkpoint protocol should be initiated, then wait on a
condition variable. Remember that the callback is called from within a
thread, so the main thread (and any other non blcr threads) continue
running.

When the mpi process enters the progress engine, it sees the flag is set
and initiates the checkpoint protocol (sending markers, shutting down
netmods, etc). When this protocol is complete, it signals the condition
variable to wake the blcr thread, then waits on its own condition
variable.

Once woken, callback thread calls the blcr function to actually take the
checkpoint. When the checkpoint completes (I can't remember if that's a
blocking function or we poll), it signals the condition variable to wake
the main thread, and returns.

When the main thread wakes it restarts the netmods and continues.

Note that the callback is also called when the process restarts after a
failure, so there is a switch statement (I believe) in there to test for
that.

## Email 2

The hydra proxy starts a blcr checkpoint of itself and all of its
children (i.e., all of the mpi processes on that node). This results in
the callback being called for itself and all of the mpi processes. In
the callback, the hydra proxy excludes itself from the checkpoint and
exits the callback. This results in checkpointing all of the processes
on one node in a single checkpoint image. Note that the callback is
actually called in a separate thread (rather than in an interrupt
context). This allows us to initiate and complete the checkpoint
protocol before taking the checkpoint of the local process.

In the callback of the mpi processes, rank 0 sets `start_checkpoint=TRUE`
then waits on `ckpt_sem`, every other process just waits on `ckpt_sem`. In
the progress engine, when `start_checkpoint=TRUE` (at rank0), it calls
`ckpt_start()`, which sends marker messages to every process. When the
other processes receive the first marker message, they call
`ckpt_start()`. Once every process receives a marker from every other
process, the `finish_checkpoint` flag is set.

Back at the progress engine, when `finish_checkpoint` is `TRUE`,
`ckpt_finish()` is called. This signals the `ckpt_sem` semaphore that the
blcr callback is waiting on then waits on the `cont_sem`.

When the `ckpt_sem` is signaled, the callback continues, and tells blcr
it's ready for blcr to take the checkpoint image. Once all processes on
the node are ready, blcr checkpoints all processes into one checkpoint
image.

When the blcr checkpoint function returns, the callback checks the
return code to determine if this is a restart, or just continuing after
taking the checkpoint. In the restart case, the environment variables
are reset (the env vars after a restart are the same as they were the in
the original process, we need new env vars for the new incarnation of
this process because, e.g., the PMI connection info will have changed.).
Then the stdin out and err pipes are reconnected to the pmi proxy (when
the pmi proxy initially starts the mpi process, it dup's the stdin out
and err fds, but when the processes are restarted, the proxy does not
fork (it calls blcr restart), so it doesn't have a chance to dup the
fds, so the process itself needs to connect the fds through named pipes
that the pmi proxy set up). Next it reinitializes pmi so new business
cards can be exchanged. Finally, it lets the netmod know that it has
restarted from a checkpoint.

In the continue case, it lets the netmod know that the checkpoint has
completed, and we're continuing.

When a marker message is received we need to make sure that no messages
are received from that VC until the checkpoint has completed. In TCP, we
handle this by holding the messages at the sender (as opposed to
blocking incoming messages at the receiver). So after the restart we
need to send control messages to tell the processes it's OK to send us
messages again.

Note that maker messages are not sent to intranode processes. This is
because blcr will include the shared memory region in the checkpoint
when processes are checkpointed together. We use the `nem_barrier()`
(which is a shared memory intranode barrier) to sync the local
processes, but otherwise don't drain the shared-memory queues.

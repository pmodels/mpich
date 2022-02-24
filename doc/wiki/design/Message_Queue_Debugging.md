# Message Queue Debugging

The capability to dump CH3 and nemesis-tcp recv/send queues from a
debugger was added in . The functions used to do so are:

``` c
 void MPIDI_CH3U_Dbg_print_recvq(FILE *stream)
```

This function dumps the state of the CH3 receive queues (both posted and
unexpected) to the given stream. A common choice for the stream is
`stdout` or `stderr`.

``` c
 void MPID_nem_dbg_print_vc_sendq(FILE *stream, MPIDI_VC_t *vc)
```

This function dumps the state of the nemesis shared memory and module
send queues for a particular VC (virtual connection). This is only
implemented for the tcp module at the moment, although it is simple to
add support for other modules as needed.

``` c
 void MPID_nem_dbg_print_all_sendq(FILE *stream)
```

This is a convenience function that basically just calls
`_print_vc_sendq` for all known VCs in all known PGs (process groups).

You can call these functions from a debugger to aid in debugging various
problems such as incorrect ordering of `MPI_Send`'s and `MPI_Recv`'s in
collective algorithms or user code. These functions are not related to
the [proper debugger interface](Debugger_Message_Queue_Access.md) that allows you to
examine the message queues from totalview and other debuggers.

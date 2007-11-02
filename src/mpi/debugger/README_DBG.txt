Information on the debugger interface

The debugger interface runs *in the debugger*.  The file dll_mpich.c
is used to create a library libtvmpich2.a .  This library must be a
dynamically loadable shared library.

The debugger provides a number of functions that are used by the
debugger interface functions to read the program image.

Working Notes

For the queues, we need a stable way of getting successive queue elements.  
We could snapshot the queues or could change the (internal) interface
to invalidate the queue information if the queue changes during a
sequence of "next" calls.

The debugger interface needs to keep a list of several items,
including communicators and all active send requests.  We need to make
handling these lists part of the object allocation/deallocation.
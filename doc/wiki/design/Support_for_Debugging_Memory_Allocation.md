# Support For Debugging Memory Allocation

## Heap Allocation Debugging

The MPICH code contains built-in support for tracing and verifying
memory allocation. To make use of this support, configure with
`--enable-g=mem` (you may combine `mem` with other debugging options,
such as `--enable-g=dbg,log,mem`).

The following environment variables, if they are set to the value `YES`
or `yes`, will change the behavior of the memory allocation routines:

  - `MPICH_TRMEM_VALIDATE`
    Validate all of the allocated memory, checking for damaged headers
    or overwritten memory sentinels, every time memory is allocated or
    freed.
  - `MPICH_TRMEM_INITZERO`
    Initialize allocated and freed memory to zero. Normally, the memory
    is initialized to `0xda` to help detect references to uninitialized
    memory. Freed memory is filled with `0xfc` to help detect
    reads/writes from/to deallocated memory.

During `MPI_Finalize`, any memory allocated but not freed will be
written out.

## Handle Allocation Debugging

MPICH also contains separate facilities for allocating object handles
(such as `MPI_Comm`'s) and the associated underlying object. Similarly,
these facilities have debugging code to help with memory errors that is
enabled via `--enable-g=mem`. You should also add `meminit` if you
intend to debug with valgrind ([see below](#valgrind-integration))

Newly allocated handles will be filled with `0xef`. Freed handles will
be filled with `0xec`.

## Valgrind Integration

The old version of the memory debugging code interfered with valgrind's
ability to detect uninitialized data. As of  MPICH contains 
[Valgrind Client Requests](http://valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.clientreq)
to prevent this problem when configured with `--enable-g=meminit`.

If `valgrind.h` and `memcheck.h` are not in your default path you will
need to add that include path to your configure line.

```
 ./configure [your_args_here] --enable-g=meminit CPPFLAGS='-I/path/to/valgrind/includes'
```

For example:

```
 ./configure --prefix=/sandbox/goodell/mpich-installed --enable-g=dbg,mem,meminit CPPFLAGS='-I/usr/include/valgrind'
```

If configure can't find these two headers then valgrind may not report
certain types of errors when `--enable-g=mem` is specified. Also, if you
intend to use valgrind with MPICH you should generally configure with
`--enable-g=dbg,meminit` at the very least. This will get you stack
traces and prevent several known false positives from being reported as
well as the false negative prevention that has been mentioned.

# Reporting And Returning Error Codes

There are two parts to reporting an error in MPICH: handling the initial
detection of the error and unwinding the stack as the error is returned.

See also [Returning Error Codes](../source_code/Coding_Standards.md#returning-error-codes) in the
discussion of coding standards.

## Creating an Error Code

When an error is detected, an error code should be created with
`MPIR_Err_create_code`. The inputs to this routine are no longer
documented, so try to figure it out from similar use in the code.

For simple cases, there are macros in
[src/include/mpierrs.h](https://trac.mcs.anl.gov/projects/mpich2/browser/mpich2/trunk/src/include/mpierrs.h)
that may help. For example,

```
if (failure) {
    MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**myname");
}
```

will set `mpi_errno` to an appropriate error code (encoding the
information implied in the name `"**myname"`; (see
[Adding New Error Messages](../how_to/Adding_New_Error_Messages.md))
and jump to the label fn_fail.

## Passing an error code up the call stack

In cases where an error code should simply be returned, such as in this
case

```
mpi_errno = MPIR_foo( .... );
```

instead of simply issuing a return as in

```
mpi_errno = MPIR_foo( .... );
if (mpi_errno) return mpi_errno;
```

you should consider providing a hook to allow the implementation to add
additional information:

```
mpi_errno = MPIR_foo( .... );
if (mpi_errno) { MPIU_ERR_POP( mpi_errno ); }
```

This assumes that the label `fn_fail` is defined and that all exits from
the routine go to this label (e.g., this may handle unwinding other
information that is maintaining while the routine is active). This is
called "POP" because it is handling a "nested" error - that is, handling
an error that was returned by another (called) routine.

The definitions of these macros are in the file
[src/include/mpierrs.h](https://trac.mcs.anl.gov/projects/mpich2/browser/mpich2/trunk/src/include/mpierrs.h).

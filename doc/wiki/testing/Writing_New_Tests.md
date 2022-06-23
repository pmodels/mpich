# Writing New Tests

A thorough test suite is an important part of any project. It is equally
important that the test suite be used frequently, and that the results
of the test suite be easy to evaluate. In MPICH, the tests are designed
to be run by scripts that are run nightly, including scripts that
analyze the results to identify success and failure. For a test to work
within this process, it must do the following:

1.  Have the process with rank zero in `MPI_COMM_WORLD` print nothing
    except the line (note the leading space; the "No" starts in the
    second column. The `^` and `$` characters are only there to
    show where the line begins and ends, respectively. This is to
    simplify Fortran tests, where the first column is sometimes
    interpreted and a blank is required for greatest portability).

```
^ No Errors$
```

    if the test is successful; no other process should print anything at
    all.
2.  Print anything else on failure (from any process).
3.  Run without any command line arguments or special environment
    variables.

In addition, to make it easier to diagnose any failures, a good test
will write detailed diagnostic information about any failure.

To aid in writing tests, there are a set of utility routines in
`test/util/mtest.c` (with corresponding versions for Fortran and C++).
These routines include ones to print the required output, check the
command line and environment for options to produce more verbose output,
and routines to print additional information when so requested. They
also provide a variety of MPI communicators and datatypes to ensure that
tests are carried out with more than `MPI_COMM_WORLD` and basic MPI
datatypes. Until there is better documentation, the best way to use
these is to look at an existing test that makes use of these routines,
such as `test/mpi/pt2pt/sendrecv1.c` or `test/mpi/pt2pt/rqstatus.c`.

To avoid unnecessary complications for users, any test should either use
the routines in `mtest.c` to check for command line options or
environment variables, implement the same options for specifying verbose
output. Specifically, support the environment variables

  - `MPITEST_DEBUG` - If set to a numeric value, turn on debugging output (the value may
    be used to control the amount of output; for many tests, any
    positive value should generate debugging output).

  - `MPITEST_VERBOSE` - If set to a numeric value, turn on verbose output. As with
    `MPITEST_DEBUG`, the value determines the amount of output.

Test programs may of course implement additional options, but for
consistency should use these where they make sense. If you want
additional features, such as command line control over verbose output,
consider enhancing the routines in `mtest.c`.

Once a test has been written, add it to the MPICH test suite by placing
it in the appropriate subdirectory of `mpich/test/mpi`. For example, a
test of `MPI_Send` should go into `mpich/test/mpi/pt2pt`. Tests in C++
or Fortran should go into their subdirectories (e.g., a Fortran test of
`MPI_Send` should go into `mpich/test/mpi/f77/pt2pt`). Add the test to
the Makefile.sm and to the testlist file in that directory. The format
of the entries in the testlist file are

```
executable-name number-of-processes
```

E.g., for a test name `sendtest` to be run on five processes, add the
line

```
sendtest 5
```

There are additional special options in the testlist file that can be
used to modify the way the test is conducted, but most tests should not
require those features.

For the `Makefile.sm` file, the most common entry that is needed is of
the form

```
mytest_SOURCES = mytest.c
```

for a program with name `mytest.c`. For more complicated needs, such as
examples that require multiple files, look for similar examples in the
`Makefile.sm` files in the test directories.

## Other Suggestions when Writing Tests

In the past, some tests have been written with code like this:

```
...
err = MPI_Send( ... );
if (err != MPI_SUCCESS) {
   ... report error;
}
```

However, if the code does not also include

```
MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
```

(and similar code for every communicator used), the code misleads the
reader into believing that the MPI routine will return with an error
code instead of abort. It is better to either not use the error return
(an abort of the code will be detected by the test harness) or set the
error handler to MPI_ERRORS_RETURN.

Another common mistake is to select among datatypes with the C switch
statement, as in

```
switch (type) {
   case MPI_INT: ...
}
```

This works for MPICH but is not portable to all valid MPI
implementations because the MPI predefined datatypes are not required to
be compile-time constants. Instead of using a switch statement, you need
to use an if-else chain:

```
if (type == MPI_INT) { ...
} else if (type ...
```

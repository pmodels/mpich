# Writing Tests

This page covers writing new tests for the MPICH test suite in `test/mpi/`
and maintaining existing ones. Internal routines that can be tested without
MPI, such as parsers and schedule generators, can instead get a unit test in
`test/units/`; see [Unit Tests](Unit_Tests.md).

A test in `test/mpi/` takes one of two forms:

- A **test function**. The test defines `int run(const char *arg)` instead
  of `main`. `runtests` runs many test functions inside one `run_mpitests`
  process, between a single `MPI_Init` and `MPI_Finalize`, which saves the
  startup cost of each test. Most C tests in `attr`, `coll`, `comm`,
  `datatype`, `errhan`, `group`, `info`, `part`, `pt2pt`, `rma` and `topo`
  are written this way. Use this
  form for a new C test unless it needs one of the exceptions listed in
  [Tests that run individually](#tests-that-run-individually).
- A **standalone program** with its own `main`. Fortran and C++ tests,
  tests in the other directories, and tests that need their own MPI
  process use this form.

## Rules for every test

Tests are run and checked by scripts (`test/mpi/runtests`), so every test
must follow these rules:

1.  Succeed silently: when the test passes, no process prints anything
    except the success line described below.
2.  Print anything else on failure, from any process. A good test prints
    enough detail to diagnose the failure.
3.  Run without command line arguments or environment variables. Options
    that a test accepts must have defaults, and the testlist passes the
    non-default ones (see [The testlist file](#the-testlist-file)).

A standalone program reports success by having the process with rank zero
in `MPI_COMM_WORLD` print exactly the line below. Note the leading space;
the "No" starts in the second column (the `^` and `$` only mark the start
and end of the line). The leading space keeps Fortran tests portable, where
the first column is sometimes interpreted.

```
^ No Errors$
```

`MTest_Finalize(errs)` prints this line for you. A test function does not
print it: it returns its error count, and `run_mpitests` does the
reporting.

Tests should use `MPITEST_DEBUG` and `MPITEST_VERBOSE` for extra output.
`MPITEST_DEBUG`, if set, turns on debugging output. `MPITEST_VERBOSE`, if
set to a numeric value, turns on verbose output, and the value controls the
amount of output. The routines in `mtest.c` read
them (e.g. `MTestPrintfMsg`). If you want more features, such as command
line control over verbose output, consider adding them to `mtest.c` rather
than to a single test.

## The MTest library

`test/mpi/util/mtest.c` and `mtest_common.c` (with Fortran and C++
versions next to them) provide helper routines, declared in
`test/mpi/include/mpitest.h`:

- `MTest_Init` and `MTest_Finalize` for standalone programs.
- Iterators over a variety of communicators, datatypes and windows, so
  that a test covers more than `MPI_COMM_WORLD` and basic types, e.g.
  `MTestGetIntracomm`, `MTestGetIntercomm` and `MTestGetWin`.
- `MTestArgList` routines to read `-key=value` options (see
  [Arguments](#arguments)).
- `MTestPrintError`, `MTestPrintfMsg` and similar output helpers.

Datatype tests can also use the DTPools library (`test/mpi/dtpools`); see
[DTPools](../design/DTPools.md). Until there is better documentation, the
best way to learn these routines is to look at an existing test that uses
them, such as `test/mpi/pt2pt/sendrecv1.c` or `test/mpi/pt2pt/icsend.c`.

## Where tests go

Put the test in the subdirectory of `test/mpi/` for the area it tests. For
example, a test of `MPI_Send` goes into `test/mpi/pt2pt`, and a Fortran
test of `MPI_Send` goes into `test/mpi/f77/pt2pt` (or `f90`, `f08`). Each
directory has:

- `Makefile.am`, which lists the test programs in `noinst_PROGRAMS`.
  Automake infers the source from the program name, so a test built from
  `mytest.c` only needs `mytest` in the list. Tests that need several
  source files or extra flags get their own `mytest_SOURCES`, `_CPPFLAGS`
  or `_LDADD` lines; look for similar examples in the existing
  `Makefile.am` files.
- `testlist.in` (or `testlist` in some directories), which lists the test
  runs. `configure` creates `testlist` from `testlist.in`, substituting
  `@...@` values; a directory that uses `testlist.in` must be listed in
  `AC_CONFIG_FILES` in `test/mpi/configure.ac`. Some directories also have
  extra lists, such as `testlist.gpu`, that are used only in some
  configurations.

A new subdirectory must also be added to the parent's `Makefile.am`
(`static_subdirs` in `test/mpi/Makefile.am`), to the parent's testlist,
and to `test/mpi/configure.ac`.

### The testlist file

Each line names a program and the number of processes, optionally followed
by `key=value` options:

```
sendtest 5
sendrecv3 2 arg=-isendrecv
probe_unexp 4 env=MPIR_CVAR_CH4_OFI_EAGER_THRESHOLD=16384 timeLimit=120
```

A program may appear on several lines with different options. The options
that `runtests` recognizes are:

| Option | Meaning |
|---|---|
| `arg=VALUE` | Pass `VALUE` to the test as an argument. Repeat for several arguments. |
| `env=NAME=VALUE` | Set an environment variable (usually a CVAR) for this run. |
| `mpiexecarg=VALUE` | Pass `VALUE` to `mpiexec`. |
| `timeLimit=SECONDS` | Override the default time limit (180 seconds, scaled by `MPITEST_TIMEOUT_MULTIPLIER`). |
| `xfail=REASON` | Expected to fail. Skipped unless `runtests` is given `-runxfail`. |
| `mem=GB` | Skip the test when less memory is available to the test run. |
| `strict=false` | Skip the test when `runtests` runs with `-strict`. |
| `resultTest=NAME` | Use another check of the output, e.g. `TestAllowWarnings`. |

Every option must have the `key=` form. A bare word such as `-isendrecv`
is silently ignored, so the test runs without it.

Most tests need none of these options. For conditions that depend on the
platform or the configuration (an option that one netmod lacks, a known
failure on one OS), don't edit the testlist; use the xfail and skip rules
in the Jenkins scripts instead (see [Jenkins](Jenkins.md)).

## Test functions

### Writing a test function

A test function looks like this (`test/mpi/pt2pt/icsend.c`, shortened):

```c
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_icsend
int run(const char *arg);
#endif

int run(const char *arg)
{
    int errs = 0;

    MTestArgList *head = MTestArgListCreate_arg(arg);
    int do_randomize = MTestArgListGetInt_with_default(head, "randomize", 0);
    MTestArgListDestroy(head);

    ...
    return errs;
}
```

The same source is compiled twice:

- Into `run_mpitests`, with `MULTI_TESTS` defined. The `#define` renames
  `run` to a unique name: the test's path relative to `test/mpi`, with `/`
  replaced by `_`. The name must match the test's entry in
  `all_mpitests.txt` (see [Registering a test function](#registering-a-test-function)).
- Into its own program, e.g. `pt2pt/icsend`, linked with
  `util/librun_mpitests.la`, which supplies a `main` that calls
  `MTest_Init`, calls `run` with the command line arguments, and prints
  `No Errors` or the error count. This program is what runs when the test
  runs individually.

Because many tests share one process, a test function must:

- Not call `MPI_Init`, `MPI_Finalize`, `MTest_Init` or `MTest_Finalize`.
- Return its error count instead of printing ` No Errors`. `run_mpitests`
  sums the counts across processes.
- Free everything it creates: communicators, groups, datatypes, windows,
  keyvals, requests, info objects and memory. Objects left behind can
  change the results of later tests. `run_mpitests` calls `MTest_Cleanup`
  after each test, which frees the communicators from `MTestCommRandomize`
  and the window keyval used by `MTestGetWin`.
- Not depend on file-scope state from an earlier run. The same function may
  run several times in one process (once per testlist line with that
  process count). Reset any file-scope variable, such as a global error
  count, at the start of `run`.
- Not rely on the error handler of `MPI_COMM_WORLD` staying set.
  `run_mpitests` resets it to `MPI_ERRORS_ARE_FATAL` before each test.
- Avoid names that clash with other tests. Make helper functions and
  file-scope variables `static`.

### Arguments

A test function receives its testlist `arg=` values as one string, e.g.
`"-randomize=1 -op=sum"`. Read them with the `MTestArgList` routines:

```c
MTestArgList *head = MTestArgListCreate_arg(arg);
int count = MTestArgListGetInt_with_default(head, "count", 100);
const char *op = MTestArgListGetString_with_default(head, "op", "sum");
MTestArgListDestroy(head);
```

Each argument must have the form `-key=value` or `-key`, with a single
dash; `-key` alone means `-key=1`. Positional arguments are not supported,
so a test that took a positional count now takes an option such as
`-loopcount=N`. Copy a string value before destroying the list.

Use arguments instead of building the same source several times with
different macros. For example, the rma tests that were built as `*_shm`
or `*_allocation` variants now take `arg=-use-win-allocate` or a similar
option.

### Registering a test function

1. Add the test's path, relative to `test/mpi`, to
   `test/mpi/maint/all_mpitests.txt`, e.g. `pt2pt/icsend`. Extra
   libraries go after the name, e.g. `coll/allgatherv4 -lm`.
2. Add the program to `noinst_PROGRAMS` in the directory's `Makefile.am`,
   and make sure the `Makefile.am` has `include Makefile_mpitests.mk`
   after `include $(top_srcdir)/Makefile_single.mtest`.
3. Add the test runs to the directory's testlist, as for any test.

`test/mpi/autogen.sh` runs `maint/gen_all_mpitests.py`, which reads
`all_mpitests.txt` and generates:

- `all_mpitests.c`, the table of test names and functions in
  `run_mpitests`;
- `Makefile_mpitests.mtest`, the rules that build `run_mpitests` from all
  the test sources;
- `<dir>/Makefile_mpitests.mk`, which links each test's own program with
  `librun_mpitests.la` (plus the extra libraries).

These files are generated and ignored by git. Rerun `test/mpi/autogen.sh`
(or the top-level `autogen.sh`) after changing `all_mpitests.txt`.

### How runtests runs them

`runtests` reads `all_mpitests.txt`. Each testlist line for a test listed
there runs in `run_mpitests`, unless the line has an option that
`run_mpitests` cannot handle: an `env=` setting that must be in place
before `MPI_Init`, an `mpiexecarg=`, or a `resultTest=`. Such a line runs
the test's own program individually. For each testlist file, `runtests`
starts one `run_mpitests` job per process count and writes the tests to
its standard input, one per line. Rank 0 reads each line and broadcasts
it, all ranks call the test function, and rank 0 writes back
`name: N` with the total error count. If `run_mpitests` aborts,
`runtests` reports the test that was running as failed and starts a new
`run_mpitests` with the next test.

An `env=` setting runs inside `run_mpitests` only for CVARs that can be
changed after `MPI_Init`: the collective algorithm CVARs (such as
`MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM`) and
`MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE`. `run_mpitests` sets them through
`MPI_T` before the test and restores them after. The list is in
`is_runtime_cvar` in `runtests`.

To run every test individually, as the suite did before `run_mpitests`,
set `MPITEST_RUN_INDIVIDUAL=1` when running `runtests` (or
`make testing`).

### Tests that run individually

Keep a test as a standalone program, or leave it out of
`all_mpitests.txt`, if it:

- tests `MPI_Init`, `MPI_Finalize`, `MPI_Init_thread`, sessions, or
  behavior before initialization or at finalize (e.g. `info/info_no_init`,
  `comm/inactive_reqs`);
- needs a thread level other than the default, or is a threads test;
- allocates very large buffers (e.g. `coll/coll_large`,
  `pt2pt/pt2pt_large`), so that a failure doesn't take the remaining tests
  with it;
- is built with special compiler flags or libraries, such as the tests
  built only without strict MPI.

A test function whose testlist lines need init-time CVARs,
`mpiexecarg=` or `resultTest=` can still be listed in `all_mpitests.txt`;
those lines run individually and the others run in `run_mpitests`.

### Running a test function by hand

From the build directory of the test (`test/mpi/<dir>`):

```sh
make icsend
mpiexec -n 4 ./icsend -randomize=1
```

The arguments are the same as the testlist's `arg=` values. To run tests
inside `run_mpitests`, give it the tests on standard input:

```sh
cd test/mpi
make run_mpitests
printf 'pt2pt/icsend -randomize=1\npt2pt/sendrecv1\n' | mpiexec -n 4 ./run_mpitests
```

`runtests -verbose` prints the lines it sends to `run_mpitests`.

## Standalone programs

A standalone program calls `MTest_Init` and `MTest_Finalize`:

```c
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;

    MTest_Init(&argc, &argv);
    ...
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
```

Add it to `noinst_PROGRAMS` in the `Makefile.am` and to the testlist. A
test that must call `MPI_Init` itself (for example, to test it) prints
` No Errors` from rank 0 itself.

Many C tests have been converted from standalone programs to test
functions (see [pmodels/mpich#5725](https://github.com/pmodels/mpich/pull/5725)).
When converting one, rename `main` to `run`, drop `MTest_Init` and
`MTest_Finalize`, turn command line handling into `MTestArgList` options
(updating the testlist `arg=` values), and follow the rules in
[Writing a test function](#writing-a-test-function).

## Other suggestions when writing tests

In the past, some tests have been written with code like this:

```
...
err = MPI_Send( ... );
if (err != MPI_SUCCESS) {
   ... report error;
}
```

However, if the code does not also include

```
MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );
```

(and similar code for every communicator used), the code misleads the
reader into believing that the MPI routine will return with an error
code instead of abort. It is better to either not use the error return
(an abort of the code will be detected by the test harness) or set the
error handler to MPI_ERRORS_RETURN.

Another common mistake is to select among datatypes with the C switch
statement, as in

```
switch (type) {
   case MPI_INT: ...
}
```

This works for MPICH but is not portable to all valid MPI
implementations because the MPI predefined datatypes are not required to
be compile-time constants. Instead of using a switch statement, you need
to use an if-else chain:

```
if (type == MPI_INT) { ...
} else if (type ...
```

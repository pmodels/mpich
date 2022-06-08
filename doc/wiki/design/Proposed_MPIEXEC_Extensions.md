# Proposed MPIEXEC Extensions

The following are proposed extension of the command-line options and
environment variables accepted by `mpiexec`. These address environment
variables, labeling of output, and return status, and make sense only in
environments where those features exist. This is a draft for discussion
only. It does describe the behavior, at least in part, of the mpd and
gforker mpiexecs. That is, some of the behavior described here is
already supported by one or more mpiexec programs.

To reduce the burden on both users (to know which command line options
to use) and on implementers (to avoid bug reports caused by confusion
over which `mpiexec` is in use), at least one person feels that it would
be a good idea to have uniform syntax and semantics for these
extensions. In particular, it would be best if an `mpiexec`
implementation did not implement similar functionality with different
syntax.

  - `-np <num>` - A synonym for the standard `-n` argument

  - `-env <name> <value>` - Set the environment variable <name> to <value> for the processes being
    run by `mpiexec`

  - `-envnone` - Pass no environment variables (other than ones specified with other
    `-env` or `-genv` arguments) to the processes being run by `mpiexec`. By
    default, all environment variables are provided to each MPI process
    (rationale: principle of least surprise for the user).

  - `-envlist <list>` - Pass the listed environment variables (names separated by commas), with
    their current values, to the processes being run by `mpiexec`.

  - `-genv <name> <value>` - The `-genv` options have the same meaning as their corresponding `-env`
    version, except they apply to all executables, not just the current
    executable (in the case that the colon syntax is used to specify
    multiple executables).

  - `-genvnone` - Like `-envnone`, but for all executables

  - `-genvlist <list>` - Like `-envlist`, but for all executables

  - `-usize <n>` - Specify the value returned for the value of the attribute
    `MPI_UNIVERSE_SIZE`.

  - `-l` - Label standard out and standard error (`stdout` and `stderr`) with the
    rank of the process

  - `-maxtime <n>` - Set a timelimit of <n> seconds.

  - `-exitinfo` - Provide more information on the reason each process exited if there is
    an abnormal exit.

  - `-stdoutbuf=type` - Set the buffering type for stdout. Type may be **none**, **line**, or
    **block**.

  - `-stderrbuf=type` - Set the buffering type for stderr. Type may be **none**, **line**,
    or **block**.

Some environment variables may be needed by the MPI or process
management system to help launch the MPI processes. These variables are
always present, even if the `-envnone` or `-genvnone` options are used.
These environment variables may include

```
PMI_FD, PMI_RANK, PMI_SIZE, PMI_DEBUG, MPI_APPNUM, MPI_UNIVERSE_SIZE,
PMI_PORT, PMI_SPAWNED
```

No other environment variables should be present if `-envnone` or
`-genvnone` are set, other than ones that the operating system provides
for every process.

Notes to implementors: Any other data can be communicated once a
connection is established between the MPI process and the process
manager. These environment variables are intended to all that connection
to take place (`PMI_FD` and `PMI_PORT`) and to control debugging
information before the connection is established (`MPI_DEBUG`). The
remaining variables are present for backward compatibility. This
definition matches the documentation provided for MPICH as of version
1.0.3 (see "Other Command-Line Arguments to mpiexec" in the User's
Manual).

Notes to users: These environment variables may or may not be present.
In particular, `MPI_APPNUM` and `MPI_UNIVERSE_SIZE` are not required.
For both of these, the value of this environment variable, if used by
the process manager or `mpiexec`, will be set by `mpiexec` and will
override any value set by the user. In other words, you cannot use

```
setenv MPI_UNIVERSE_SIZE 100
mpiexec a.out
```

to run `a.out` with a universe size of 100.

## Environment variables for mpiexec

The following environment variables are understood by some versions of
`mpiexec`. The command line arguments have priority over these; that is,
if both the environment variable and command line argument are used, the
value specified by the command line argument is used.

  - `MPIEXEC_TIMEOUT` - Maximum running time in seconds. `mpiexec` will
    terminate MPI programs that take longer than the value specified by
    `MPIEXEC_TIMEOUT`.

  - `MPIEXEC_UNIVERSE_SIZE` - Set the universe size

  - `MPIEXEC_PORT_RANGE` - Set the range of ports that `mpiexec` will use in communicating with the
    processes that it starts. The format of this is <low>`:`<high>. For
    example, to specify any port between 10000 and 10100, use `10000:10100`.

  - `MPICH_PORT_RANGE` - Has the same meaning as `MPIEXEC_PORT_RANGE` and is used if
    `MPIEXEC_PORT_RANGE` is not set.

  - `MPIEXEC_PREFIX_DEFAULT` - If this environment variable is set, output to standard output is
    prefixed by the rank in `MPI_COMM_WORLD` of the process and output to
    standard error is prefixed by the rank and the text `(err)`; both are
    followed by an angle bracket (`>`). If this variable is not set, there
    is no prefix.

  - `MPIEXEC_PREFIX_STDOUT` - Set the prefix used for lines sent to standard output. A `%d` is
    replaced with the rank in `MPI_COMM_WORLD`; a `%w` is replaced with an
    indication of which `MPI_COMM_WORLD` in MPI jobs that involve multiple
    `MPI_COMM_WORLD`s (e.g., ones that use `MPI_Comm_spawn` or
    `MPI_Comm_connect`).

  - `MPIEXEC_PREFIX_STDERR` - Like `MPIEXEC_PREFIX_STDOUT`, but for standard error.

  - `MPIEXEC_STDOUTBUF` - Set the buffering type for stdout. Type may be **NONE**, **LINE**,
    or **BLOCK**.

  - `MPIEXEC_STDERRBUF` - Set the buffering type for stderr. Type may be **NONE**, **LINE**,
    or **BLOCK**.

## Return Status

`mpiexec` returns the maximum of the exit status values of all of the
processes created by `mpiexec`, with the status values defined as an
unsigned int. On many systems, the status value may be a smaller
integer, such as an unsigned char.

## Support for Multithreaded and Multicore Applications

In multithreaded applications, it can be important to both place the
processes and threads carefully (so that processor resources are
available to the threads) and to communicate to the thread library how
many threads should be used for thread parallelism (to avoid
over-subscribing the node because there may be multiple MPI processes on
the node).

A draft proposal, titled "MPIT: Requirements for a Common Runtime
Environment for Multi-process, Multi-threaded Applications," has been
circulated. This section describes some thoughts on support for
`mpiexec` for achieving the same aims. Among the differences are the use
of distinct environment variable names for values that may be different
at each MPI process (to allow for process managers that cannot provide
different values) and a clear separation between what are called "User
Variables" and "Runtime Variables" in that document (where variables
with the same name may have different values).

  - `MPIT_PROCMAP` - Provides information about the number of CPUs available to each MPI
    process. This could be a comma separated list of items of the form

```
first:last[:stride]-ncpu
```

where `first` and `last` are ranks (in `MPI_COMM_WORLD`) of processes,
`stride` is an increment, and `ncpu` is the number of CPUs for *each* of
the processes. For example,

```
0:4-2,5:7:2-1,6:8:2-4
```

produces this arrangement:

| rank | \# of CPUs |
| ---- | ---------- |
| 0    | 2          |
| 1    | 2          |
| 2    | 2          |
| 3    | 2          |
| 4    | 2          |
| 5    | 1          |
| 6    | 4          |
| 7    | 1          |
| 8    | 4          |

  - `MPIT_CPUS` - The number of processors (CPUS) available to this process. If the
    process manager does not support providing each process with a unique
    value for each environment variable, then **MPIT_CPUS_Rrank**, where
    `rank` is the rank of the process in `MPI_COMM_WORLD`, may be used
    instead.

    When processes are created with `MPI_Comm_spawn` or
    `MPI_Comm_spawn_multiple`, this information can be provided though the
    `info` argument to those routines. The natural info keys are the
    lower-case versions of the environment variables.

    Another issue that applies to both multithreaded applications and to
    multiple processes on the same node is that of processor affinity - how
    are threads and processes mapped only processors? For some memory-bound
    applications, it can be important to bind their threads to a single
    processor. In other situations, particularly when load balancing is
    paramount, it is important *not* to bind threads to a single processor.
    To provide this one hint, we could consider

  - `MPIT_BIND_TO_PROCESSOR_SET` - If this value is true, the MPI processes are bound to some processors.
    Note that this refers to the MPI process; if the MPI process is
    multithreaded, and the desire of the programmer is for each thread to
    have a processor, then the process must be bound to a set of processors
    (the size of the set can be determined from **MPIT_CPUS**).

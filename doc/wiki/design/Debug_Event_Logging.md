# Debug Event Logging

## Overview and Rationale

A uniform mechanism for logging important events in the execution of
code can be a tremendous help in debugging the code. The approach
described here should be used with new code. This code is active if the
log option is specified to `--enable-g` when MPICH is configured (we
recommend `--enable-g=dbg,log` when developing MPICH).

If MPICH is already configured to including logging and you wish to
create logs, the following environment variable settings are
recommended:

``` bash
setenv MPICH_DBG FILE
setenv MPICH_DBG_LEVEL VERBOSE
```

This causes each MPI process to generate a separate file, numbered by
rank in `MPI_COMM_WORLD`. If there are multiple worlds or multiple
threads, the file names will contain the "world number" and the thread
id. Alternately, you can run the program with these two additional
options:

```
-mpich-dbg=file -mpich-dbg-level=verbose
```

The script `mpich/src/util/dbg/getfuncstack` may be used to
"pretty-print" the log files. For example

``` bash
cd examples
mpiexec -n 4 ./cpi -mpich-dbg=file -mpich-dbg-class=thread,routine
...
../src/util/dbg/getfuncstack < dbg0.log | more
```

will show you where thread-related operations occur within the routines.

### Log Content

The log file looks like following:

```
0       -1      7f4b384d6740[604321]    1       0.000064        src/mpi/datatype/typerep/src/typerep_yaksa_init.c       411     Entering MPIR_Typerep_init
0       -1      7f4b384d6740[604321]    2       0.000264        src/mpi/datatype/typerep/src/typerep_yaksa_init.c       439     Leaving MPIR_Typerep_init
...
```

One line for each log entry. Each line uses the format:

```
world_num  world_rank   threadID[pid]  class  curtime   file   line   [log message]
```

The first entry get logged is after `MPII_pre_init_dbg_logging()`. The
rank field are logged as -1 until `MPII_init_dbg_logging()` call, which
is after `MPID_Init()`.

### Quickstart Settings

*from goodell@* The settings that I frequently use are:

``` bash
export MPICH_DBG_FILENAME="log/dbg-%w-%d.log"
export MPICH_DBG_CLASS=ALL
export MPICH_DBG_LEVEL=VERBOSE
```

Another set that is really great for debugging nemesis dynamic process
issues is:

``` bash
export MPICH_DBG_FILENAME=log/dbg-%w-%d-%p.log
export MPICH_DBG_CLASS=ROUTINE,COMM,NEM_SOCK_DET,CH3_CONNECT,CH3_DISCONNECT
export MPICH_DBG_LEVEL=VERBOSE
```

## Adding an Event

There are five macros that may be used to add an event to the log. In
most cases, the first macro is sufficient.

``` c
 MPIU_DBG_MSG(class,level,string)
 MPIU_DBG_MSG_S(class,level,format-with-%s,string)
 MPIU_DBG_MSG_D(class,level,format-with-%d,int)
 MPIU_DBG_MSG_P(class,level,format-with-%p,pointer)
 MPIU_DBG_MSG_FMT(class,level,(MPIU_DBG_FDEST,format,args))
```

e.g.,

``` c
 MPIU_DBG_MSG(THREAD,TERSE,"Getting lock");
```

The strange format for the general case allows the macro to implement
the general format by using snprintf into a local string. In addition,
the following macro allows a statement (or a block) to be controlled in
the same way:

``` c
MPIU_DBG_STMT(class,level,statement)
```

For more complex situations, you can use the following template:

``` c
#ifdef USE_DBG_LOGGING
    if (MPIU_DBG_SELECTED(class,level)) {
       ... any code, including
       MPIU_DBG_OUT(class,string)
       MPIU_DBG_OUT_FMT(class,(MPIU_DBG_FDEST,....));
    }
#endif
```

The macros `MPIU_DBG_OUT` and `MPIU_DBG_OUT_FMT` are similar to the
`MPIU_DBG_MSG` and `MPID_DBG_MSG_FMT` macros, except that they do not
take a level argument, and their expansions perform no tests on the
level or class values. These are appropriate when, for example, using a
loop to write out multiple lines of data.

In all of these macros, the first two values are used to control which
actions are active. These are discussed in the next section.

## Event Classes and Levels

The predefined classes are

  - `PT2PT` - Point-to-point communication
  - `RMA` - Remote memory access communication
  - `THREAD` - Thread-related actions, such as entering or leaving a critical
    section
  - `PM` - Process-management related action
  - `ROUTINE` - Routine entrance and exit (these may be part of the routine entrance
    and exit macros).
  - `ALL` - All (any) class

Multiple classes may be combined with the or `|` operator; internally,
the class is stored as a bitmask and the test is on any bit set. The
predefined message levels are:

  - `TERSE` - Intended for messages that are always helpful when debugging
  - `TYPICAL` - Intended for messages that are often helpful
  - `VERBOSE` - Intended for detailed debugging

For example, when all "typical" message are requested, all messages with
level "typical" and below are logged; e.g., both TERSE and TYPICAL
messages are logged. Both the predefined classes and levels are prefixed
with `MPIU_DBG_`; the macro implementation adds the prefix (with the
\#\# preprocessor operation) as part of the macro expansiong.

## Controlling the logging package

The general format for controlling the debugging package is the same for
both command line and environment variable versions:

  - Command line - `\-mpich-dbg-<name>=value`
  - Environment variables - `MPICH_DBG_<NAME>`

The names and their meaning are:

  - `filename=pattern`
    Pattern contains `%w` for the world number, `%d` for rank in world,
    `%p` for pid, and `%t` for thread id. In addition, the conditionals
    `@W...@` and `@T...@` can be used to include a value only when there
    is more than one world or thread respectively (this is the same
    syntax used by the generic mpiexec labeling code in
    `src/pm/util/labelout.c`. For example


``` bash
setenv MPICH_DBG_FILENAME "dbg-%w-%d.log"
```

  - It is usually best if each thread logs events to a separate file;
    this avoids any problem with file system coherence. The default
    filename pattern is `dbg@W%w-@%d@T-%t@.log`. For an MPI-1
    application with a single thread, this creates output files for the
    form (here for rank=3):

```
dbg3.log
```

  - For an MPI-2 dynamic application with 2 worlds and several threads,
    the filename would be

```
dbg1-3-43534.log
```

  - (for the rank=3 process in world=1 with thread id 43534).

  - The special pattern, `-stdout-`, will send all output to stdout.
    Likewise, `-stderr-` will send all output to stderr. An empty
    pattern will have the same effect as specifying `-stdout-`.  One way
    to deal with the collision from the buggy `%w` code is to use the
    `%p` specifier to cause the process ID (pid) to be substituted.
  - level=name
    Set the level of messages. For example

```
mpiexec a.out -mpich-dbg-level=typical
```

  - `class=name\[,name\]`
    Turn on one or more classes of debugging output. For example

``` bash
setenv MPICH_DBG_CLASS ROUTINE,THREAD
```

  - `rank=val`
    Select one rank in MPI_COMM_WORLD for logging. This is
    particularly useful when looking at the routine trace for a
    particular sequence of operations or to avoid problems caused by
    writing data from multiple processes to stdout.

These are processed by a hook in the implementation of
`MPI_Init_thread`.

As a shorthand, the values

```
-mpich-dbg
setenv MPICH_DBG yes
```

may be used to turn on debugging with common options; this will normally
be the "typical" level and all classes, sent to stdout.

### Implementation Notes

The logging output format is roughly the following:

```
world\trank-in-world\tthread-id\ttimestamp\tfile\tline\tmsg
```

where `\t` is a tab character.

A simple implementation of `MPIU_DBG_MSG` might be

``` c
#define MPIU_DBG_MSG(_class,_level,_string) \
    if ((MPID_DBG_##_class & MPIU_DBG_ACTIVECLASSES) && \
         MPID_DBG_##_level <= MPIU_DBG_MAXLEVEL) {\
        fprintf( stderr, "%d\t%d\t%d\t%f\t%s\t%d\t%s\n", \
        MPIU_DBG_WORLDNUM, MPIU_DBG_RANK, MPIU_DBG_THREADNUM, \
        PMPI_Wtime(), __FILE__, __LINE__, _string ); fflush( stderr );\
    }
```

This assumes that `MPID_DBG_WORLDNUM` etc. are global variables that are
accessible to the file, and (for the multi-threaded case) that fprintf
is thread-safe. The actual implementation is more likely to look like

``` c
#define MPIU_DBG_MSG(_class,_level,_string) \
    if ((MPID_DBG_##_class & MPIU_DBG_ACTIVECLASSES) && \
         MPID_DBG_##_level <= MPIU_DBG_MAXLEVEL) {\
        MPIU_DBG_Outevent( __FILE__, __LINE__, _string ); }
```

where `MPIU_DBG_Outevent` is a routine that handles the output of the
string (this will be a varargs routine, so that all of the macros above
will use the same routine).

An additional feature that we may wish to consider (and which was
implemented within MPICH-1) is a circular buffer of log events that is
only output on failure (either an `MPID_Abort` or a signal such as `SIGINT`
or `SIGSEGV`). This could be selected with the name "lastonly" as in

```
mpiexec -n 4 a.out -mpich-dbg-lastonly=500
```

(Only output the last 500 lines for each thread). Interpreting the Logs
The script, dbgextract, in `src/util/dbg` is intended for use with
`-mpich-dbg-class=routine`. It reads a log file and outputs a simplified
version that indents the routine states according to their nesting level
as executed, making it easier to see the trace of execution in MPICH.

### Rationale for Design Choices

  - Why not use varadic macros (macros that allow a variable number of
    arguments)? They are part of the C99 standard.
    Some important compilers, including the Portland Group compilers, do
    not support this feature yet (there are many compilers that do not
    support all of C99 yet).

## Using the Older Debugging Code

To turn on the `MPIDI_DBG_PRINTF` debugging macro, you need to compile
with the flag `MPICH_DBG_OUTPUT` (do `setenv CFLAGS -DMPICH_DBG_OUTPUT`
before running configure) and at runtime you need
to set the environment variable `MPICH_DBG_OUTPUT` to either `stdout`,
`file`, or `memlog`.

Note that this interface will be removed in a subsequent release of
MPICH in favor of the MPIU_DBG_MSG interface described above.

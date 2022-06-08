# Making MPICH Thread Safe

## Thread Safety

In a multi-threaded MPI program, care must be taken to ensure that
updates and accesses to data structures shared between threads in the
same process. This applies to both the use of multiple threads within
the MPI implementation, e.g. to support background processing of
messages, and to user-threads that make MPI calls (the
`MPI_THREAD_MULTIPLE` case).

The classical approach to thread-safety is to establish *critical
sections* around the parts of the code that must be executed atomically.
This is often done using locks, but other approaches are possible. In
some cases, it is possible to use careful coding or special processor
instructions to ensure that an update is atomic (for example, an atomic
increment or compare-and-swap). A major consideration is what
granularity of critical section (or of locks, which are more general) to
use.

In the implementation of MPICH, two different approaches are used for
mediating the access to shared structures.

1.  [Critical sections](#critical-sections "wikilink"). These are used
    to guard shared data structures and system calls, such as queue
    updates and reads and writes to sockets.
2.  [Atomic update and access](#atomic-reference-and-update").
    These are used for simple operations that can be implemented with
    processor-atomic operations. This is usually applied to reference
    count updates that only require an integer update.
3.  [Memory consistency](#memory-consistency). There are some
    issues that need to be considered, particularly when narrowing the
    scope of the critical sections.

In addition, there are two special cases that need to be handled
efficiently.

1.  [One-time Initialization](#one-time_initialization)
2.  [The Progress Engine](#the-progress-engine)

Finally, in order to provide a common set of routines for basic thread
operations, there are a number of 
[common thread routines](#common-thread-api).

To simplify the source code, macros are used for both atomic updates and
critical sections. When MPICH is configured for single-threaded
operation (both `MPI_THREAD_SERIALIZED` or lower and only one thread
used by the MPICH implementation itself), these macros turn into the
appropriate single threaded code. Specifically, in the single-threaded
case, the atomic update and reference operations use non-atomic (and
less expensive) versions and the critical section macros turn into
no-ops.

### Critical Sections

**This document is obsolete**. While there is much good stuff in this
document, the MPICH code base makes use of a variation on the macros and
definitions in this document. In particular, all critical sections are
now marked with macros that take the name of the critical section and a
reference to the object as arguments, permitting a single set of macros
to implement different granularity of thread safety. This version
describes the code used in release 1.0.7, but not the version in the
thread branch. That branch will soon be merged into the main version.

The new form of the macros uses these names to delimit critical sections

- `MPIU_THREAD_CS_ENTER(name,context)` - Enter a critical section
  called `name` for a particular context (e.g., communicator)
- `MPIU_THREAD_CS_EXIT(name,contest)` - Exit the named critical
  section.

These are sufficient for most uses that require atomic access or update.
There are two other major needs: lightweight atomic updates, such as
reference counters, and sharing of a resource. For lightweight atomic
updates, there are the atomic operations and the reference-count update
macros. For resources, the following is a proposed design.

- `MPIU_THREAD_RESOURCE_ENTER(csname,resourcename)` - Gain exclusive
  access to a resource associated with a critical section names
  `csname`. Release the critical section.
- `MPIU_THREAD_RESOURCE_EXIT(csname,resourcename)` - Release the
  resource, re-entering the critical section
- `MPIU_THREAD_RESOURCE_POKE(csname,resourcename)` - Provides a hook to
  poke or wakeup the thread that has acquired the resource. For
  example, in the current MPICH sock implementation, there is an FD
  that is local to the process; each of the threads, by writing a byte
  to this fd, cause the thread that is doing a `poll` on the fds to
  wake up. By abstracting this concept, parts of the implementation
  that may need to cause the thread holding the resource to wake up
  can do so, allowing that thread to use blocking operations.

Please edit this file and bring it up-to-date (rather than creating a
new version).

Old text begins here.

There are two obvious approaches to thread safety in MPICH: (1) the
single, global critical section and (2) finer grain (object- or
module-based) access. Because we want to experiment with and understand
the different approaches, we propose to use the following sets of macros
to describe thread-synchronization points within MPICH:

1.  A single critical section. This is often the first approach used in
    a thread-safe application, and has the advantage of simplicity. The
    disadvantage is that, as a coarse-grain approach, threads that are
    in fact using distinct data structures may be waiting on the global
    critical section when they don't need to.
    For this approach, there are two major macros:
      - `MPIU_THREAD_SINGLE_CS_ENTER(msg)` - Enter the single critical section
      - `MPIU_THREAD_SINGLE_CS_EXIT(msg)` - Exit the single critical section
    The `msg` argument is a string (it may be empty) that may be used
    when debugging. This is one special case that needs to be considered
    which is the handling of the creation of error codes. MPICH provides
    instance-specific error codes; these require use of a shared data
    structure. Rather than force all code to be within a critical
    section in case an error is discovered, even code that otherwise
    would not need a critical section, such as code that only references
    data such as `MPI_Comm_rank`, the error code routines have their own
    thread-safe support. This will be discussed
    [later](#Error_Codes "wikilink").
2.  Per module and/or per object critical sections (and atomic update
    operations). This is a more complex approach but it can offer
    greater performance by avoiding unnecessary critical sections and by
    allowing separate threads to run concurrently when possible.
    For this approach, there are two sets of macros:
        - `MPIU_THREAD_<class>_CS_ENTER[(obj)]` - Enter the critical section
          for the class. Depending on the class, this may be a per-class or per
          object within the class operation. In the latter case, the object is
          passed to the macro.
        - `MPIU_THREAD_<class>_CS_EXIT[(obj])` - Exit the critical section for
          the class or the object within the class.

The classes still need to be defined, but they may include:

- COLL - Collective
- COMM - Communication, including progress
- MEM - Memory and object allocation/deallocation
- LIST - Reference or update to a list of objects

See Analysis of Thread Safety Needs of MPI Routines for one set of
possible classes.

One special object pointer is `&MPIR_Process`; this provides a "master
lock" that can be used when there is no relevant object.

When one of these two approaches is chosen, the macros for the other are
defined as empty. This allows us to annotate the source code with
detailed information about the needed thread synchronization without
requiring separate source code versions or complex ifdefs.

In addition to these two macros to enter and exit the critical section,
additional macros are needed to aid in declarations and initialization:

- `MPIU_THREAD_SINGLE_CS_DECL` - Provide the declarations needed for the
  single critical section. This should be placed where it is globally
  accessible (e.g., within the `MPIR_Process` structure).
- `MPIU_THREAD_SINGLE_CS_INITIALIZE` - Initialize the single critical section.
  This should appear in the `MPI_Init`/`MPI_Init_thread` routines.
- `MPIU_THREAD_SINGLE_CS_FINALIZE` - Finalize the single critical section.
  This should appear in the `MPI_Finalize` routine.
- `MPIU_THREAD_SINGLE_CS_ASSERT_WITHIN(msg)` - This is a special version of an
  *assert* that can be used to check that the critical section is indeed held.

For the finer-grain approach, these are the macros:

- `MPIU_THREAD_<class>_CS_DECL` - Provide the declarations needed. This should
  either be within the object declaration (when the granularity is per object)
  or in the `MPIR_Process` structure (when the granularity is per class).
- `MPIU_THREAD_<class>_CS_INITIALIZE[(obj)]` - Initialize the critical section.
- `MPIU_THREAD_<class>_CS_ASSERT_WITHIN[(obj)]` - This is a special version of an
  *assert* that can be used to check that the critical section is indeed held.

One possible problem with threaded code that uses more than on critical
section is deadlock caused when two or more threads both need to acquire
two or more critical sections, and each acquire a critical section
needed by the other, causing them to wait for the other to release the
critical section. This is called a *deadly embrace*.

The following is a possible way to help debug these cases. For example,
each critical section could increment a "critical section" counter (in
thread-private storage); if this value exceeds one, then an error can be
raised. A more sophisticated approach could define a "priority"; a low
priority critical section may not be held when attempting to acquire a
higher priority critical section. These checks would be enabled when
testing and debugging but during performance runs. This does not change
the API described in this note; it only changes the implementation.

There are two other major classes of operations that must be handled in
a thread safe way: initialization and reference count updates. In
addition, there is code that is needed only when thread-support is
included.

### One-time Initialization

Some routines need to be initialized the first time they are executed.
Initializations are often controlled by a flag that is cleared once the
initialization is completed. In a single-threaded case, the code is
often simply

```
static int initNeeded = 1;
...
if (initNeeded) {
    code to initialize
    initNeeded = 0;
}
```

In the multi-threaded case, it is possible that two threads will enter
the `if (initNeeded)` at nearly the same time. We could avoid this by
putting the test on `initNeeded` within a critical section, but then
every entry into this routine will need to acquire and release the
critical section, a potentially costly operation. A very simple approach
is to use two tests, with the second inside a critical section:

```
static volatile int initNeeded = 1;
...
if (initNeeded) {
    MPIU_THREAD_SINGLE_CS_ENTER("");
    if (initNeeded == 1) {
         code to initialize
         MemoryBarrier;
         initNeeded = 0;
    }
    MPIU_THREAD_SINGLE_CS_EXIT("");
}
```

However, the full generality of a critical section is not needed, so in
some cases, an approach that uses two test variables is used, with the
first (fast) test checking the `initNeeded` flag and a second test or an
atomic fetch and increment on the flag. For example,

````
static volatile int initNeeded = 1;
static volatile int initLock = 0;
...
if (initNeeded) {
    MemoryBarrier;
    AtomicFetchandIncrement(initLock);
    if (initLock == 0) {
        code to initialize
        MemoryBarrier;
        initNeeded = 0;
    } else {
        /* Spin, waiting for other thread to init */
        while (initNeeded) ;
    }
}
```

Note that several `MemoryBarrier`s are needed, and that some mechanism
(here an atomic fetch-and-increment) is needed to make sure only one
thread performs the initialization. The positive feature of this is that
once the initialization is performed, no costly thread locks or atomic
memory operations are required. Because it can be tricky to get the
thread-safe features (such as the memory barriers correct) and because
the single-threaded case does not require anything this complex, there
are special macros for this case:

```
MPIU_THREADSAFE_INIT_DECL(var);
...
MPIU_THREADSAFE_INIT_STMT(var,code to initialize)
```

if the code to initialize is simple (no commas) or the following for
more complex initialization code:

```
MPIU_THREADSAFE_INIT_DECL(var);
...
if (var) {
    MPIU_THREADSAFE_INIT_BLOCK_BEGIN(var);
    code to initialize
    MPIU_THREADSAFE_INIT_CLEAR(var);
    MPIU_THREADSAFE_INIT_BLOCK_END(var);
}
```

The `MPIU_THREADSAFE_INIT_CLEAR` is important; it ensures that the
necessary memory barrier can be issued before clearing the variable that
controls the initialization check.

To understand these macros, the following table shows how each may be
implemented in the context of the example above:

|                                          |                                    |
| ---------------------------------------- | ---------------------------------- |
| `MPIU_THREADSAFE_INIT_DECL(var);`        | `static volatile int var=1;`       |
| `if (var) {`                             | `if (var) {`                       |
| `MPIU_THREADSAFE_INIT_BLOCK_BEGIN(var);` | `MPIU_THREAD_SINGLE_CS_ENTER("");` |
| *code to initialize*                     | *code to initialize*               |
| `MPIU_THREADSAFE_INIT_CLEAR(var);`       | `var=0;`                           |
| `MPIU_THREADSAVE_INIT_BLOCK_END(var);`   | `MPIU_THREAD_SINGLE_CS_EXIT("");`  |
| `}`                                      | `}`                                |

An example where this is needed is in the default version of
`MPID_Wtick`, where the value of the clock tick is determined on the
first call to the routine.

### Atomic Reference and Update

In some cases, a simple atomic reference or update is needed. There are
three cases:

1. Reference is outside of all critical sections
2. Reference is within the single critical section case, but outside of
   the finer grain critical sections.
3. Reference is within both the coarse-grain and fine-grain critical
   sections.

Different implementation are needed for each of these cases.

However, before we further pursue this approach, we will determine
whether there is a real need for it, based on our experiences with the
two approaches to critical sections in the code. If it turns out that
all (or even most) of the atomic references and updates are already
within a critical section, we won't need to use this approach. Note that
one place where we *can* use this is in the 
[creation on instance-specific error codes](#error-codes).

Fortunately, the first case is rare and can in fact be defined away by
requiring that all such updates be moved within the single critical
section. Thus, we need only distinguish between the cases where we are
within the single critical section but outside of the particular object
or module's critical section and where we are within the critical
section.

The following macros should be used to update the reference count for an
MPIU object (e.g., an MPI communicator or group) in the second case:
within the single critical section but outside of the object's critical
section:

```
MPIU_Object_set_ref( MPIU_Object * )
MPIU_Object_add_ref( MPIU_Object * )
MPIU_Object_release_ref( MPIU_Object *, int *inuse )
```

Normally, the `MPIU_Object_add_ref` is used to increase the reference
count and `MPIU_Object_release_ref` is used to decrease the reference
count and in addition sets the flag `inuse` to true if the reference
count is positive. `MPIU_Object_set_ref` is use to initialize the
reference counter. For the third case, we could simply reference the
object directly, since we would be sure that the reference was within
the critical section. However, to emphasize the need for an atomic
update, we define a similar set of macros:

```
MPIU_Object_In_<class>`_set_ref( MPIU_Object * )`
MPIU_Object_In_<class>`_add_ref( MPIU_Object * )`
MPIU_Object_In_<class>`_release_ref( MPIU_Object *, int *inuse )`
```

These make it easy to track changes to the object's reference counts by
changing the definitions of these macros.

### Handling Thread-Only Code

In some cases, there is code that is used only when thread support is
enabled. We define two cases. For the general case, we define a simple
name on which we can test:

```
#ifdef MPICH_IS_THREADED
    MPIU_THREAD_CHECK_BEGIN
    ...
    MPIU_THREAD_CHECK_END
#endif
```

The two lines, `MPIU_THREAD_CHECK_BEGIN` and `MPIU_THREAD_CHECK_END`,
are used in executable code only (not in a declaration) to provide a
hook to allow runtime checks on whether the MPICH implementation is
multi-threaded. This will allow us to support running with a lower level
of thread safety (e.g., `MPI_THREAD_FUNELLED`) than the library
supports, and thus allow us to perform experiments on the overhead of
adding thread-safety support.

For simple blocks of code, we can avoid the use of the ifdef by using a
macro that is defined to give its argument only if `MPICH_IS_THREADED`
is defined:

```
MPIU_ISTHREADED(statement);
```

In most of MPICH, only at the thread level of `MPI_THREAD_MULTIPLE` is
thread-related code needed, and thus `MPICH_IS_THREADED` is only defined
for `MPI_THREAD_MULTIPLE.` There are exactly two places where special
thread code is needed for `MPI_THREAD_SERIALIZED;` these are in
`MPI_Is_thread_main` and in `MPI_Init_thread`. In these cases, the test
that should be used is

```
#if MPICH_THREAD_LEVEL >= MPI_THREAD_SERIALIZED
   ...
#endif
```

### Memory Consistency

When one thread updates two or more memory locations, neither the
underlying programming language (except for some very recent,
thread-aware languages) nor the processor hardware guarantees that all
other threads will see the updates in the same order. Even in a
cache-coherent system, variables that are not defined as `volatile` in C
may be stored in register and if the code uses the value in register
instead of loading from memory, changes to that variable may by another
thread will not be seen.

Implementations of critical sections take care of everything *except*
the `volatile` issue by ensuring that all memory operations have
completed; for example, it may issue a memory fence instruction on
processors that re-order memory loads and stores to force all pending
memory updates to complete. When writing code that avoids using critical
sections, you should consider whether `volatile` or a memory fence or
some similar operation may be needed. The current macros described here
do not include a memory fence because, at least at the level of the
single critical section code, we're not convinced one is needed. In
other words, where such a fence may be needed, there is already a
critical section that ensures the memory consistency. Note that the
[atomic update](#atomic-reference-and-update)
operations may need a memory fence within their implementation.

### The Progress Engine

The progress engine is responsible for advancing communication. There
are two natural approaches for implementing a thread-safe progress
engine:

1. A separate thread is responsible for running the progress engine.
   Typically, other (i.e., user) threads enqueue requests to this
   thread.
2. Any thread may invoke the progress engine; there is no separate
   thread that runs the progress engine.

A key requirement on the progress engine in both of these cases is that
the thread running the progress engine must not block all threads when
it makes a blocking system call. This means that it must release any
critical sections it may hold before calling the blocking system call,
and it must reacquire the critical sections when the blocking system
call returns.

To handle this case, the progress engine provides a special API made up
of a set of routines that are used to invoke the progress engine. These
apply either to the single critical section or to the finer-grain
communication class critical section.

- `MPID_Progress_start(state)` - Enter a state where only the calling
  thread will modify communication objects (e.g., request completion
  flags will not change)
- `MPID_Progress_wait(state)` - Wait for any event handled by the progress
  engine. This allows the calling thread to block rather than spin-wait.
- `MPID_Progress_test()` -  A nonblocking version of `MPID_Progress_wait`
- `MPID_Progress_end(state)` - Exit the state begun with `MPID_Progress_start`
- `MPID_Progress_poke()` - Give the progress engine an opportunity to run
  without invoking `MPID_Progress_start`.

Here is a sample of how the Progress routine API is used to implement
`MPI_Wait`:

```
MPI_Wait( ... )
{
    while (request->busy) {
        MPID_Progress_start( state );   // Notes that we are about to
                                        // check ready flags.  No busy 
                                        // flags will be cleared
        if (request->busy) {
            MPID_Progress_wait( state );
        }
        else {
            MPID_Progress_end();
        }
    }
}
```

Note: this is drawn from the old MPICH design document and may not
reflect the current implementation.

The progress routines `MPID_Progress_start` and `MPID_Progress`) end
calls are analogous to the
[critical section](#critical-sections) enter and exit macros described
above. The `MPID_Progress_wait` is a special call that allows the
progress engine to release the progress critical section, permitting a
different thread to complete a progress call. This call only returns
when the progress engine has detected some progress.

Internally to the implementation of the progress engine, additional
routines are needed to manage the coordination between different threads
that call the progress engine.

In the single-threaded case, the progress start and end calls may be
macros that expand into empty statements (thus avoiding the cost of a
function call).

Note: The interface to the progress engine may change to reduce the
latency of common cases, for example, by saying "wait until this request
or array of requests is complete".

## Error Codes

When MPICH is configured to produce instance-specific error messages
(this is the default), the messages are stored in a ring of message
buffers. The index into this ring is computed with an atomic fetch and
increment operation (mod the ring size). To avoid any possibility of
conflict with thread locks in other parts of the code, the error code
and message routines use their own thread-safe macros, even if the rest
of the MPICH code is using a single critical section.

In order to make the code as robust as possible, the error code and
message routines attempt to use an atomic fetch and increment operation
if one is available for the processor. If not, a standard thread mutex
(lock) is used; this lock is separate from any lock used in any other
part of the code. Note that since this code is not performance critical
but must be robust, the design here aims for simplicity over
performance.

## Rationales

This section contains a discussion of the rationales for some of the
choices.

### Why a single ifdef test for thread support?

In order to ensure that the source code remains consistent, it is
important to establish some clear coding guidelines. For example, we
want to avoid something like the following:

```
#ifdef thread_level > MPI_THREAD_SINGLE && MPICH_IMPLS_THREADS
...
#endif 
... many lines of code
#ifdef thread_level > MPI_THREAD_SINGLE
...
#endif 
```

where the second `ifdef` is missing the second test.

### Why not include the if in the one-time initialization macros?

An alternate design for the one-time initialization macros could simply
use

```
MPIU_THREADSAFE_INIT_BLOCK_ENTER(needsInit);
   ... perform initialization action
MPIU_THREADSAFE_INIT_BLOCK_END(needsInit);
```

but this requires familiarity with these macros when browsing the code.
By keeping the initial if check, the condition under which the code is
active is clear, even if this requires the use of an additional
`MPIU_THREADSAFE_INIT_CLEAR` macro.

### Common Thread API

In order to simplify the MPICH code, there is an "MPICH" API for thread
utilities.

- `MPID_Thread_tls_get` - Access thread-private storage
- `MPID_Thread_tls_set` - Set a value in thread-private storage
- `MPE_Thread_self` - Return the id of the thread
- `MPE_Thread_same` - Compare two thread ids and indicate whether the 
  threads are the same. This allows the thread-id to be an arbitrary value.
- `MPE_Thread_yield` - Allow this thread to be de-scheduled.
- `MPE_Thread_mutex_create` - Create a thread mutex
- `MPE_Thread_mutex_lock` - Lock a thread mutex
- `MPE_Thread_mutex_unlock` - Unlock a thread mutex
- `MPE_Thread_mutex_destroy` - Destroy a thread mutex
- `MPE_Thread_tls_get` - Access thread-private storage. tls stands for
  "thread local storage"
- `MPE_Thread_tls_set` - Set a value in thread-private storage

### Why are there two sets of definitions for the reference count macros?

If the operation is not within a critical section, it will be
implemented either with a memory-atomic processor instruction or by
acquiring and releasing a thread lock. Both of these are more expensive
than the simple C code (e.g., an atomic memory increment of the variable
a is more expensive than a simple `a++;`).

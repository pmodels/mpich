# Sharing Blocking Resources

## Sharing A Blocking Resource Among Threads

One important operation in the MPICH code (and in many threaded codes)
is sharing a blocking resource, such as an I/O operation. This has the
property that (a) no lock should be held across the blocking operation
and (b) at most one thread at a time should use the blocking operation.
In addition, there is sometimes a need to force the thread that holds
the resource out of the resource (in the current sock code, this is done
with a write to an fd that is in the set on which the thread is
waiting).

It is also important to be able to test whether some thread holds the
resource, and to efficiently wait for the resource to become free.

An abstraction that is often used in this situation is a *monitor*.
Simple monitors provide a fair way to share a critical region of code.
More complex monitors provide ways to share methods, including providing
for treating some threads as more urgent than others.

Questions:

1.  In the current global-only mode, the global lock is held outside of
    any monitor.

Here are more details on the various operations, divided by whether a
thread gains access to the resource immediately or not

For the thread that acquires the resource:

1.  Enter (or remain) within a critical section to access/update shared
    state information
2.  Note that the resource is now in use (atomic because the thread is
    within the critical section), release critical section, and enter
    blocking resource
3.  When blocking routine returns, reacquire critical section, mark
    resource as no longer in use, and access/update shared state
    information
4.  When all updates are complete, allow a waiting thread to access the
    resource

For the thread that discovers that the resource is in use and needs it:

1.  Wait for access to the resource (gain access to the critical
    section, check whether resource is available or not)

For threads that do something that needs to interrupt the blocking
resource

1.  Poke the thread holding the resource

Thus, there are two different but related abstractions:

1.  Atomic sharing of a blocking resource. For this, we have
    `MPIU_THREAD_RESOURCE_xxx` macros
2.  Efficiently sharing access to a critical section. This is a classic
    situation for a monitor. For this, we have `MPIU_THREAD_MONITOR`.

STILL TO BE DONE

Here is a set of abstractions to deal with this.

```
MPIU_THREAD_RESOURCE_SHARE(_name,_context,_resource)
MPIU_THREAD_RESOURCE_REENTER(_name,_context,_resource)
MPIU_THREAD_RESOURCE_POKE(_name,_context,_resource)
```

### Example

For example, in the global-mutex case, these would be used as

```
MPIU_THREAD_RESOURCE_SHARE(ALLFUNC,sockset,SOCKPOLL)
n = poll(...)
MPIU_THREAD_RESOURCE_REENTER(ALLFUNC,sockset,SOCKPOLL)
```

### Implementation

For example, in the case of sockets as the communication mechanism and
threads controlled with a global mutex (which must be released around
the shared function, a poll in the case of the MPICH sock channel), the
definitions might look like this:

```
#define MPIU_THREAD_RESOURCE_SHARE(_name,_context,_resource) \
    MPIU_THREAD_RESOURCE_SHARE_##_name##_##_resource(_context)
#define MPIU_THREAD_RESOURCE_REENTER(_name,_context,_resource) \
    MPIU_THREAD_RESOURCE_REENTER_##_name##_##_resource(_context)
#define MPIU_THREAD_RESOURCE_POKE(_name,_context,_resource) \
    MPIU_THREAD_RESOURCE_POKE_##_name##_##_resource(_context)
```

and

```
#define MPIU_THREAD_RESOURCE_SHARE_ALLFUNC_SOCKPOLL(_context) \
            (_context)->SHARE_InUse = 1; \
        MPID_Thread_mutex_unlock(&MPIR_ThreadInfo.global_mutex);
#define MPIU_THREAD_RESOURCE_REENTER_ALLFUNC_SOCKPOLL(_context) \
        MPID_Thread_mutex_lock(&MPIR_ThreadInfo.global_mutex); \
            (_context)->SHARE_InUse = 0;
#define MPIU_THREAD_RESOURCE_POKE_ALLFUNC_SOCKPOLL(_context) \
            MPID_SockPoke( _context )
```

(Note that this requires the use of non-recursive locks)

### Reflections On Early Implementations

The MPICH `ch3:sock` channel implemented the resource sharing over all of
the sockets in several places. The primary ones are in
`mpid/common/sock/poll/sock_wait.i` and `socki_util.i`, where the global
mutex is released and re-acquired around the poll call, and in
`mpid/ch3/channels/sock/src/ch3_progress.c`, where a separate set of
variables are used to test whether the code is blocked in a poll call
and provide for a condition variable to notify a thread that is waiting
to enter the resource.

### Questions and Issues

In the case where thread support is optional, we might want to include
those tests within these macros. However, that is more difficult,
particularly if blocking and nonblocking access to the shared resource
is desired.

### Status

This has not been implemented; it is posted here for comments.

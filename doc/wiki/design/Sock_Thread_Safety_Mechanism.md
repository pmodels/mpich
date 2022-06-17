# Socket Thread Safety Mechanism

This document tries to explain how the thread safety mechanism works in
the sock channel. Other channels (e.g., nemesis) use a thread safety
mechanism based on this.

## Overview

A thread acquires a mutex (`MPIU_THREAD_SINGLE_CS_ENTER()`) when it
enters a top-level MPI function. If the MPI function is blocking, the
thread must release the mutex to avoid blocking other threads. The sock
channel releases the mutex before calling a blocking `poll()` system
call and reacquires the mutex when it returns. Before releasing the
mutex, the thread sets `MPIDI_CH3I_progress_blocked` to true, and after
reacquiring the mutex the thread sets it to false. The sock channel
allows only one thread to enter the progress engine at a time in order
to avoid multiple threads calling `poll()` at the same time.

Before entering the main progress loop, a thread checks if another
thread is already in the progress loop waiting on `poll()` having
released the mutex. The `MPIDI_CH3I_progress_blocked` flag indicates
this. If there is another thread in the progress loop, the blocked
thread calls `MPIDI_CH3I_Progress_delay()` where it waits on a condition
variable (`MPIDI_CH3I_progress_completion_cond`) for the thread to leave
the main progress loop. When a thread leaves the main progress loop, it
increments the completion counter by calling
`MPIDI_CH3_Progress_signal_completion()`, then calls
`MPIDI_CH3I_Progress_continue()` to wake any threads waiting on the
`MPIDI_CH3I_progress_completion_cond` condition variable. When these
threads wake they return from the progress function to check if the
request they are waiting on has been completed.

The completion counter is needed because of the semantics of the
`pthread_cond_wait` call. Namely, it serves as the predicate
`(cached_completion_count == MPIDI_CH3I_progress_completion_count)` that
protects against spurious wakeup. From the man page:

> When using condition variables there is always a Boolean predicate
> involving shared variables associated with each condition wait that is
> true if the thread should proceed. Spurious wakeups from the
> `pthread_cond_timedwait()` or `pthread_cond_wait()` functions may
> occur. Since the return from `pthread_cond_timedwait()` or
> `pthread_cond_wait()` does not imply anything about the value of this
> predicate, the predicate should be re-evaluated upon such return.

In order to handle the case where one thread is blocking on `poll()`
while another thread completes the request that thread was waiting on, a
dummy message is sent on a *wake socket*.

The wake socket is one of the sockets that the progress engine polls on.
By sending a message on that socket, any thread waiting on `poll()` is
woken allowing it to check whether its request has completed. The wakeup
message is sent in `MPIDI_CH3_Progress_signal_completion()` if a thread
is blocked in the progress engine (i.e., `MPIDI_CH3I_progress_blocked ==
TRUE`) and a wakeup message has not already been sent (i.e.,
`MPIDI_CH3I_progress_wakeup_signalled == FALSE`).
`MPIDI_CH3I_progress_wakeup_signalled` is set to true in
`MPIDI_CH3_Progress_signal_completion()` just before sending the wakeup
message, and is set to false after the thread waiting in `poll()`
reacquires the mutex.

**Files:** The check for `MPIDI_CH3I_progress_blocked` is in
`mpid/ch3/channels/sock/src/ch3_progress.c`. The release before the poll
is in `mpid/common/sock/poll/sock_wait.i`. The wakeup pipe between
threads is in `sock_set.i`, `socki_util.i`, and other files in
`mpid/common/sock/poll`.

## Pseudocode

```
progress_wait() {
    completion_count = MPIDI_CH3I_progress_completion_count;
    /* if there's another thread already blocking, wait for it to complete a request */
    if (MPIDI_CH3I_progress_blocked == TRUE) {
        while (completion_count == MPIDI_CH3I_progress_completion_count) /* done in MPIDI_CH3I_Progress_delay() */
            cond_wait(&MPIDI_CH3I_progress_completion_cond, &MPIR_ThreadInfo.global_mutex);
        /* progress has been made, return and check if our request was completed */
        return;
    }

    /* there's no other thread blocked in the progress loop, we can enter */
    while (completion_count == MPIDI_CH3I_progress_completion_count) {
        /* release the mutex while were blocked in poll() */
        MPIDI_CH3I_progress_blocked = TRUE;
        mutex_release();
        poll(); /* if another thread completes a request, a wake message will be sent to wake this thread */
        mutex_acquire();
        MPIDI_CH3I_progress_blocked = FALSE;
        MPIDI_CH3I_progress_wakeup_signalled = FALSE;

        ... handle message ...
        /* MPIDI_CH3_Progress_signal_completion() will be called when handling a message if a
           request has been completed.  MPIDI_CH3_Progress_signal_completion() will increment
           MPIDI_CH3I_progress_completion_count. */
    }
    cond_broadcast(&MPIDI_CH3I_progress_completion_cond); /* done in MPIDI_CH3I_Progress_continue() */
}
```

## Macros

  - `MPICH_IS_THREADED` - Defined if threading is enabled

  - `HAVE_RUNTIME_THREADCHECK` - Defined if thread level is checked at
    run-time as opposed to being compiled in. With run-time thread
    checking the thread level is set when the application calls
    `MPI_Init_thread()`. When run-time thread checking is enabled, the
    global variable `MPIR_ThreadInfo.isThreaded` controls whether
    threads have been enabled.

  - `MPIU_THREAD_CHECK_BEGIN` and `MPIU_THREAD_CHECK_END` - These are
    convenience macros that can be used to check if threading is
    enabled. They can be used for both run-time thread checking and
    compile-time thread checking.

## Global Variables

  - `int MPIDI_CH3I_progress_blocked` - This variable indicates that
    there is already another thread inside the progress engine and that
    that thread is blocked waiting for an incoming message.

  - `int MPIDI_CH3I_progress_wakeup_signalled` - This variable
    indicates whether `MPIDI_CH3I_Progress_wakeup()` has been called
    since the thread which was blocking became unblocked.

  - `MPID_Thread_cond_t MPIDI_CH3I_progress_completion_cond` -
    Condition variable on which threads sleep waiting for the thread
    that's currently inside the progress engine to finish.

  - `int MPIR_ThreadInfo.isThreaded` - This is a global variable
    indicating that the application has requested thread-multiple. It is
    used in runtime thread checking.

  - `MPID_Thread_mutex_t MPIR_ThreadInfo.global_mutex` - This is the
    global mutex.

## Functions

  - `MPIDI_CH3I_Progress_wakeup()` - Called from
    `MPIDI_CH3_Progress_signal_completion()` only if another thread is
    currently blocked on `poll()`. This sends a dummy message to a
    wake-up socket. This is used to unblock a thread that's waiting (on
    `poll()`) for incoming messages.

  - `MPIDI_CH3_Progress_signal_completion()` - Called any time a
    request has completed, to indicate that all threads should check all
    requests in which they are interested to determine if any have
    completed.

  - `MPIDI_CH3I_Progress_delay()` - If a thread calls progress while
    another thread is already in the progress engine, this function is
    called. This function basically keeps waiting on a condition
    variable until some request has completed. Note that this means that
    the thread that's blocked in the progress engine must increment the
    completion counter (by calling `MPIDI_CH3_Progress_signal_completion()`)
    when it leaves the main progress loop, otherwise, you could have a 
    situation where threads are waiting in `MPIDI_CH3I_Progress_delay()` 
    but no thread is in the main progress loop.

  - `MPIDI_CH3I_Progress_continue()` - This is called by a thread to
    wake any other threads that are waiting to make progress (in
    `MPIDI_CH3I_Progress_delay()`). It is called when a thread exits the
    main progress loop. Note that this is not sufficient for releasing
    threads from `MPIDI_CH3I_Progress_delay()`. The completion counter
    must also be incremented (by calling
    `MPIDI_CH3_Progress_signal_completion()`) before calling
    `MPIDI_CH3I_Progress_continue()`.

  - `MPIU_THREAD_SINGLE_CS_ENTER()` and `MPIU_THREAD_SINGLE_CS_EXIT()` -
    Acquires or releases global mutex. In order to avoid trying to reacquire 
    the mutex if an MPI function is called from within another MPI function, 
    the *nest* level is incremented each time a top-level MPI function is called.
    The nest level is stored in thread-specific storage.

## Questions

1.  Under what conditions is wakeup needed? For example, a different
    thread cancels a request. Are there other situations? *Dave: It is
    difficult to say for sure all the places where we are performing
    wakeup right now. I have listed some possible places above, but that
    list is probably a superset of locations where wakeup is strictly
    needed.*
2.  Do All requests call signal_completion whenever the request is
    completed? Does this imply a significant overhead for all requests?
    *Dave: Yes, but signal_completion doesn't always call wakeup, so
    the overhead is likely minimal.*
3.  What is the use of the completion counter? What are all of the
    situations under which the completion counter is changed?
4.  What is (and what should be) the scope of the variables and
    routines? How many are outside of the sock-specific code? For those,
    what should be the proper abstraction to avoid sock-specific hooks
    in the MPICH code?
5.  Should "wakeup" be called only when a request is locally completed
    but might be pending (e.g., in cancel)? *Dave: As mentioned
    previously, there are legitimate non-cancel situations where we
    should call wakeup. Sending to self in various scenarios is the most
    obvious one, as are generalized requests. Probing and unexpected
    requests is also a possibility, although it will require some
    experimentation to be sure.*

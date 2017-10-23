---
layout: page
title: PMIx_Commit(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx\_Fence[_nb] - Execute a blocking[non-blocking] barrier across the processes identified in the
specified array.

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx\_Fence(const pmix\_proc\_t procs[], size_t nprocs,
                         const pmix\_info\_t info[], size_t ninfo);

pmix\_status\_t PMIx\_Fence\_nb(const pmix\_proc\_t procs[], size_t nprocs,
                            const pmix\_info\_t info[], size_t ninfo,
                            pmix_op_cbfunc_t cbfunc, void *cbdata);


{% endhighlight %}

# ARGUMENTS

*procs*
: An array of pmix\_proc\_t structures defining the processes that will be
participating in the fence collective operation.

*nprocs*
: Number of pmix\_proc\_t structures in the _procs_ array

*info*
: An optional array of pmix\_info\_t structures

*ninfo*
: Number of pmix\_info\_t structures in the pmix\_info\_t array

# DESCRIPTION

Execute a blocking[non-blocking] barrier across the processes identified in the
specified array. Passing a _NULL_ pointer as the _procs_ parameter
indicates that the barrier is to span all processes in the client's
namespace. Each provided pmix\_proc\_t struct can pass PMIX\_RANK\_WILDCARD to
indicate that all processes in the given namespace are
participating.

The info array is used to pass user requests regarding the fence
operation. This can include:

(a) PMIX\_COLLECT\_DATA - a boolean indicating whether or not the barrier
    operation is to return the _put_ data from all participating processes.
    A value of _false_ indicates that the callback is just used as a release
    and no data is to be returned at that time. A value of _true_ indicates
    that all _put_ data is to be collected by the barrier. Returned data is
    cached at the server to reduce memory footprint, and can be retrieved
    as needed by calls to PMIx\_Get(nb).

    Note that for scalability reasons, the default behavior for PMIx_Fence
    is to _not_ collect the data.

(b) PMIX\_COLLECTIVE\_ALGO - a comma-delimited string indicating the algos
    to be used for executing the barrier, in priority order. Note that
    PMIx itself does not contain any internode communication support. Thus,
    execution of the _fence_ collective is deferred to the host resource
    manager, which are free to implement whatever algorithms they choose.
    Thus, different resource managers may or may not be able to comply with
    a request for a specific algorithm, or set of algorithms. Marking this
    info key as _required_ instructs the host RM that it should return
    an error if none of the specified algos are available. Otherwise, the RM
    is to use one of the specified algos if possible, but is free
    to use any of its available methods to execute the operation if none of
    the specified algos are available.

(c) PMIX\_TIMEOUT - maximum time for the fence to execute before declaring
    an error. By default, the RM shall terminate the operation and notify participants
    if one or more of the indicated procs fails during the fence. However,
    the timeout parameter can help avoid "hangs" due to programming errors
    that prevent one or more procs from reaching the "fence".


# RETURN VALUE

Returns PMIX\_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.


# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES


# SEE ALSO

[`PMIx_Put`(3)](pmix_put.3.html),
[`PMIx_Commit`(3)](pmix_commit.3.html),
[`PMIx_Constants`(7)](pmix_constants.7.html)



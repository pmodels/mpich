---
layout: page
title: PMIx_Abort(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx_Abort - Abort the specified processes

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx\_Abort(int status, const char msg[],
                         pmix\_proc\_t procs[], size_t nprocs);

{% endhighlight %}

# ARGUMENTS

*status*
: Status value to be returned. A value of zero is permitted by PMIx, but may not be returned by some resource managers.

*msg*
: A string message to be displayed

*procs*
: An array of pmix\_proc\_t structures defining the processes to be aborted. A _NULL_ for the proc array indicates that all processes in the caller's
nspace are to be aborted. A wildcard value for the rank in any structure indicates that all processes in that nspace are to be aborted.

*nprocs*
: Number of pmix\_proc\_t structures in the _procs_ array


# DESCRIPTION

Request that the provided array of procs be aborted, returning the
provided _status_ and printing the provided message. A _NULL_
for the proc array indicates that all processes in the caller's
nspace are to be aborted.

The response to this request is somewhat dependent on the specific resource
manager and its configuration (e.g., some resource managers will
not abort the application if the provided _status_ is zero unless
specifically configured to do so), and thus lies outside the control
of PMIx itself. However, the client will inform the RM of
the request that the application be aborted, regardless of the
value of the provided _status_.

Passing a _NULL_ msg parameter is allowed. Note that race conditions
caused by multiple processes calling PMIx_Abort are left to the
server implementation to resolve with regard to which status is
returned and what messages (if any) are printed.


# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.

# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES


# SEE ALSO

---
layout: page
title: PMIx_Get(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx\_Get[\_nb] - Retrieve data that was pushed into the PMIx key-value
store via calls to _PMIx\_Put_ and _PMIx\_Commit_.

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx\_Get(const pmix\_proc\_t *proc, const char key[],
                       const pmix\_info\_t info[], size_t ninfo,
                       pmix\_value\_t **val);

pmix\_status\_t PMIx_Get_nb(const pmix\_proc\_t *proc, const char key[],
                          const pmix\_info\_t info[], size_t ninfo,
                          pmix\_value\_cbfunc_t cbfunc, void *cbdata);

{% endhighlight %}

# ARGUMENTS

*proc*
: A pointer to a pmix\_proc\_t structure identifying the namespace and rank of the
proc whose data is being requested. Note that a _NULL_ value is permitted if
the specified _key_ is unique within the PMIx key-value store. This is provided
for use by the backward compatibility APIs and is _not_ recommended for use by
native PMIx applications.

*key*
: String key identifying the information. This can be either one of the PMIx defined
attributes, or a user-defined value

*info*
: An optional array of pmix\_info\_t structures

*ninfo*
: The number of pmix\_info\_t structures in the specified _info_ array.

*val*
: Address where the pointer to a pmix\_value\_t structure containing the data to
be returned can be placed. Note that the caller is responsible for releasing
the malloc'd storage. The _PMIX\_VALUE\_FREE_ macro is provided for this purpose.

# DESCRIPTION

Retrieve information for the specified _key_ as published by the process
identified in the given pmix\_proc\_t, returning a pointer to the value in the
given address.

The blocking form of this function will block until
the specified data has been pushed into the PMIx key-value store via a call to
_PMIx\_Commit_ by the specified process. The caller is
responsible for freeing all memory associated with the returned value when
no longer required.

The non-blocking form will execute the callback function once the specified
data becomes available and has been retrieved by the local server. Note that
failure of the specified process to _put_ and _commit_ the requested data
can result in the callback function never being executed.

The info array is used to pass user requests regarding the get
operation. This can include:

(a) PMIX_TIMEOUT - maximum time for the get to execute before declaring
    an error. The timeout parameter can help avoid "hangs" due to programming
    errors that prevent the target proc from ever exposing its data.


# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.

# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES

See '_pmix\_common.h' for definition of the pmix\_value\_t structure.

# SEE ALSO
[`PMIx_Put`(3)](pmix_put.3.html),
[`PMIx_Commit`(3)](pmix_commit.3.html),
[`PMIx_Constants`(7)](pmix_constants.7.html),
[`PMIx_Structures`(7)](pmix_structures.7.html)

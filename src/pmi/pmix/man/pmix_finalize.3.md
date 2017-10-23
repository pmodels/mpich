---
layout: page
title: PMIx_Finalize(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx_Finalize - Finalize the PMIx Client

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx\_Finalize(const pmix\_info\_t info[], size_t ninfo);

{% endhighlight %}

# ARGUMENTS

*info*
: An optional array of pmix\_info\_t structures

*ninfo*
: Number of pmix\_info\_t structures in the pmix\_info\_t array

# DESCRIPTION

Finalize the PMIx client, closing the connection with the local PMIx server and releasing all malloc'd memory.

The info array is used to pass user requests regarding the fence
operation. This can include:

(a) PMIX\_EMBED\_BARRIER - By default, _PMIx\_Finalize_ does not include an internal barrier operation. This attribute directs _PMIx\_Finalize_ to execute a barrier as part of the finalize operation.

# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.

# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES


# SEE ALSO

[`PMIx_Init`(3)](pmix_init.3.html)

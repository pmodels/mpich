---
layout: page
title: PMIx_Commit(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx\_Commit - Push all previously _PMIx\_Put_ values to the local PMIx server.

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx_Commit(void);

{% endhighlight %}

# ARGUMENTS

*none*

# DESCRIPTION

This is an asynchronous operation - the library will immediately
return to the caller while the data is transmitted to the local
server in the background


# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.


# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES


# SEE ALSO

[`PMIx_Put`(3)](pmix_put.3.html)

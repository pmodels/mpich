---
layout: page
title: PMIx_Init(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx_Init - Initialize the PMIx Client

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx_Init(pmix\_proc\_t *proc);

{% endhighlight %}

# ARGUMENTS

*proc*
: Pointer to a pmix\_proc\_t object in which the client's namespace and rank are to be returned.

# DESCRIPTION

Initialize the PMIx client, returning the process identifier assigned
to this client's application in the provided pmix\_proc\_t struct.
Passing a value of _NULL_ for this parameter is allowed if the user
wishes solely to initialize the PMIx system and does not require
return of the identifier at that time.

When called, the PMIx client will check for the required connection
information of the local PMIx server and will establish the connection.
If the information is not found, or the server connection fails, then
an appropriate error constant will be returned.

If successful, the function will return PMIX_SUCCESS and will fill the
provided structure with the server-assigned namespace and rank of the
process within the application. In addition, all startup information
provided by the resource manager will be made available to the client
process via subsequent calls to _PMIx\_Get_.

Note that the PMIx client library is referenced counted, and so multiple
calls to PMIx_Init are allowed. Thus, one way to obtain the namespace and
rank of the process is to simply call PMIx_Init with a non-NULL parameter.

# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.

# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES


# SEE ALSO

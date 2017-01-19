---
layout: page
title: PMIx_Put(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx_Put - Push a value into the client's namespace

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

pmix\_status\_t PMIx\_Init(pmix\_scope\_t scope, const char key[], pmix\_value\_t *val);

{% endhighlight %}

# ARGUMENTS

*scope*
: Defines a scope for data "put" by PMI per the following:

(a) PMI_LOCAL - the data is intended only for other application
                  processes on the same node. Data marked in this way
                  will not be included in data packages sent to remote requestors
(b) PMI_REMOTE - the data is intended solely for application processes on
                   remote nodes. Data marked in this way will not be shared with
                   other processes on the same node
(c) PMI_GLOBAL - the data is to be shared with all other requesting processes,
                   regardless of location

*key*
: String key identifying the information. This can be either one of the PMIx defined
attributes, or a user-defined value

*val*
: Pointer to a pmix\_value\_t structure containing the data to be pushed along with the type
of the provided data.

# DESCRIPTION

Push a value into the client's namespace. The client library will cache
the information locally until _PMIx\_Commit_ is called. The provided scope
value is passed to the local PMIx server, which will distribute the data
as directed.

# RETURN VALUE

Returns PMIX_SUCCESS on success. On error, a negative value corresponding to
a PMIx errno is returned.

# ERRORS

PMIx errno values are defined in `pmix_common.h`.

# NOTES

See 'pmix\_common.h' for definition of the pmix\_value\_t structure.

# SEE ALSO
[`PMIx_Constants`(7)](pmix_constants.7.html),
[`PMIx_Structures`(7)](pmix_structures.7.html)

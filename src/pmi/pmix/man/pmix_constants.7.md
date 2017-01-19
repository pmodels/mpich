---
layout: page
title: PMIx Constants(7)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx Constants

# SYNOPSIS

{% highlight c %}
#include <pmix_common.h>
{% endhighlight %}


# OVERVIEW

PMIx relies on the following types of constants:

*Maximum Sizes*
: In order to minimize malloc performance penalties, PMIx utilizes constant-sized arrays wherever possible. These constants provide the user with the maximum size of the various array types.

*Attributes*
: .

*Errors*
: PMIx uses negative error constants, with 0 indicating "success".

# MAXIMUM SIZES

The .

*PMIX_MAX_NSLEN*
: The maximum length of a namespace. Note that any declaration of an array to hold a key string must include one extra space for the terminating _NULL_.

*PMIX_MAX_KEYLEN*
: Maximum length of the key string used in structures such as the _pmix_info_t_. Note that any declaration of an array to hold a key string must include one extra space for the terminating _NULL_.

# ATTRIBUTES

Define a set of "standard" PMIx attributes that can be queried using the PMIx\_Get function. Implementations (and users) are free to extend as desired - thus, functions calling PMIx\_Get must be capable of handling the "not found" condition. Note that these are attributes of the system and the job as opposed to values the application (or underlying programming library) might choose to expose - i.e., they are values provided by the resource manager as opposed to the application. Thus, these keys are RESERVED for use by PMIx, and users should avoid defining any attribute starting with the keyword _PMIX_.

A list of the current PMIx attributes, and the type of their associated data value, is provided here.

*PMIX_ATTR_UNDEF (NULL)*
: Used to initialize an attribute field, indicating that the attribute has not yet been assigned.

*PMIX_USERID (uint32_t)*
: .

*PMIX_GRPID (uint32_t)*
: An access domain represents a single logical connection into a
  fabric.  It may map to a single physical or virtual NIC or a port.
  An access domain defines the boundary across which fabric resources
  may be associated.  Each access domain belongs to a single fabric
  domain.

*PMIX_CPUSET (char\*)*
: .


# ERROR CONSTANTS

.

*PMIX_SUCCESS*
: Indicates that the operation was successful.

*PMIX_ERROR*
: A general error code - an error occurred, but no specific reason can be provided.


# SEE ALSO

[`pmix`(7)](pmix.7.html)

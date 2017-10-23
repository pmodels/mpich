---
layout: page
title: PMIx_Initialized(3)
tagline: PMIx Programmer's Manual
---
{% include JB/setup %}

# NAME

PMIx\_Initialized - Check if  _PMIx\_Init_ has been called

# SYNOPSIS

{% highlight c %}
#include <pmix.h>

int PMIx_Initialized(void);

{% endhighlight %}

# ARGUMENTS

*none*

# DESCRIPTION

Check to see if the PMIx Client library has been intialized



# RETURN VALUE

Returns _true_ if the PMIx Client has been initialized, and _false_ if not.

# ERRORS


# NOTES


# SEE ALSO

[`PMIx_Init`(3)](pmix_init.3.html)

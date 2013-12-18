## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/util/cvar/mpich_cvars.c

if MAINTAINER_MODE
# normally built by autogen.sh, but this rebuild rule is here
$(top_srcdir)/src/util/cvar/mpich_cvars.c: $(top_srcdir)/maint/extractcvars
	( cd $(top_srcdir) && ./maint/extractcvars --dirs="`cat ./maint/cvardirs`")
endif MAINTAINER_MODE

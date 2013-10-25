## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/util/cvar/mpich_cvars.c

dist_noinst_DATA += src/util/cvar/cvars.yml

if MAINTAINER_MODE
# normally built by autogen.sh, but this rebuild rule is here
$(top_srcdir)/src/util/cvar/mpich_cvars.c: $(top_srcdir)/src/util/cvar/cvars.yml $(top_srcdir)/maint/gencvars
	( cd $(top_srcdir) && ./maint/gencvars )
endif MAINTAINER_MODE

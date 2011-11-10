## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/util/param/param_vals.c

dist_noinst_DATA += src/util/param/params.yml

if MAINTAINER_MODE
# normally built by maint/updatefiles, but this rebuild rule is here
src/util/param/param_vals.c: src/util/param/params.yml
	./maint/genparams
endif MAINTAINER_MODE


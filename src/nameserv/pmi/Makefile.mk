## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_NAMEPUB_PMI

lib_lib@MPILIBNAME@_la_SOURCES +=   \
    src/nameserv/pmi/pmi_nameserv.c

endif BUILD_NAMEPUB_PMI


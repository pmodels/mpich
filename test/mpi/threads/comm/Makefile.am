## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/threads/Makefile_threads.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = ctxdup dup_leak_test comm_dup_deadlock comm_create_threads comm_create_group_threads

## this was present in the simplemake version, but I think we expect mpicc (or
## whatever is in $(CC)) to handle this for us
##ctxdup_LDADD = -lpthread 


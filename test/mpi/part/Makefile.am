##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =  \
    start_pready      \
    start_pready_wait \
    start_pready_range      \
    start_pready_list       \
    start_pready_list_non_consecutive       \
    parrived                       \
    parrived_wait                  \
    parrived_pready_out_of_order   \
    pingping                       \
    multipart                      \
    nonblocking_pready             \
    no_start


start_pready_wait_CPPFLAGS = -DTEST_WAIT $(AM_CPPFLAGS)
start_pready_wait_SOURCES  = start_pready.c

start_pready_range_CPPFLAGS = -DTEST_PREADY_RANGE $(AM_CPPFLAGS)
start_pready_range_SOURCES  = start_pready.c

start_pready_list_CPPFLAGS = -DTEST_PREADY_LIST $(AM_CPPFLAGS)
start_pready_list_SOURCES  = start_pready.c

start_pready_list_non_consecutive_CPPFLAGS = -DTEST_WAIT -DTEST_PREADY_LIST_NON_CONSECUTIVE $(AM_CPPFLAGS)
start_pready_list_non_consecutive_SOURCES  = start_pready.c

parrived_wait_CPPFLAGS = -DTEST_WAIT $(AM_CPPFLAGS)
parrived_wait_SOURCES  = parrived.c

parrived_pready_out_of_order_CPPFLAGS = -DTEST_PREADY_OUT_OF_ORDER $(AM_CPPFLAGS)
parrived_pready_out_of_order_SOURCES  = parrived.c

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest
LDADD += -lm

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = \
    p2p_latency \
    p2p_bw \
    get_bw \
    put_bw \
    p2p_one \
    barrier \
    bcast

.def.c:
	mydef_page $<

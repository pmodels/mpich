# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = \
    rdwrord       \
    rdwrzero      \
    getextent     \
    setinfo       \
    setviewcur    \
    i_noncontig   \
    async         \
    async_any     \
    userioerr     \
    resized       \
    resized2      \
    bigtype       \
    hindexed_io   \
    simple_collective


if BUILD_MPIX_TESTS
noinst_PROGRAMS +=      \
    i_bigtype           \
    i_hindexed_io       \
    i_rdwrord           \
    i_setviewcur        \
    i_aggregation1      \
    i_aggregation2      \
    i_coll_test         \
    i_darray_read       \
    i_hindexed          \
    i_noncontig_coll    \
    i_noncontig_coll2   \
    i_types_with_zeros
endif

clean-local:
	-rm -f testfile testfile.*

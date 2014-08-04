# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

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
    hindexed_io

clean-local:
	-rm -f testfile testfile.*

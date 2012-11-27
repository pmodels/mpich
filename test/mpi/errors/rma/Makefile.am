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
noinst_PROGRAMS =       \
    winerr              \
    winerr2             \
    cas_type_check      \
    win_sync_unlock     \
    win_sync_free_pt    \
    win_sync_free_at    \
    win_sync_complete   \
    win_sync_lock_at    \
    win_sync_lock_pt    \
    win_sync_lock_fence \
    win_sync_nested     \
    win_sync_op


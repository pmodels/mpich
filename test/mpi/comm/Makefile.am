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
noinst_PROGRAMS =     \
    commcreate1       \
    dup               \
    dupic             \
    dup_with_info     \
    ic1               \
    ic2               \
    commname          \
    ctxalloc          \
    ctxsplit          \
    icgroup           \
    icsplit           \
    iccreate          \
    cmfree            \
    icm               \
    cmsplit           \
    cmsplit2          \
    cmsplit_type      \
    comm_create_group \
    comm_group_half   \
    comm_group_rand   \
    probe-intercomm   \
    comm_idup         \
    comm_idup_mul     \
    comm_idup_overlap \
    comm_idup_isend   \
    comm_idup_nb      \
    comm_idup_iallreduce \
    comm_idup_comm    \
    comm_idup_comm2   \
    comm_create_group_idup \
    comm_info

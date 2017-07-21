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
noinst_PROGRAMS = cfree ccreate1 manysplit userdup too_many_comms too_many_comms2 too_many_comms3 too_many_icomms too_many_icomms2 \
comm_create_test comm_create_group_test comm_dup_test comm_get_info_test comm_group_test comm_remote_size_test comm_size_test comm_split_test \
comm_split_type_test intercomm_create_test

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
noinst_PROGRAMS = cfree \
                  ccreate1 \
                  manysplit \
                  userdup \
                  too_many_comms \
                  too_many_comms2 \
                  too_many_comms3 \
                  too_many_icomms \
                  too_many_icomms2 \
                  comm_create_nullarg \
                  comm_create_group_nullarg \
                  comm_get_info_nullarg \
                  comm_group_nullarg \
                  comm_size_nullarg \
                  comm_split_nullarg \
                  comm_split_type_nullarg \
                  intercomm_create_nullarg \
                  subcomm_abort \
                  subcomm_abort2 \
                  intercomm_abort

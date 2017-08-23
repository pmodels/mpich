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
noinst_PROGRAMS = gerr \
                  group_difference_nullarg \
                  group_intersection_nullarg \
                  group_range_excl_nullarg \
                  group_range_incl_nullarg \
                  group_rank_nullarg \
                  group_size_nullarg \
                  group_translate_ranks_nullarg \
                  group_union_nullarg


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
noinst_PROGRAMS = getcnterr type_contiguous_test type_create_resized_test type_extent_test type_get_extent_test type_get_true_extent_test /
type_get_true_extent_x_test type_lb_test type_ub_test type_size_x_test type_vector_test


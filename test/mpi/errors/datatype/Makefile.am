##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = getcnterr \
                  type_contiguous_nullarg \
                  type_get_extent_nullarg \
                  type_get_true_extent_nullarg \
                  type_get_true_extent_x_nullarg \
                  type_size_x_nullarg \
                  type_vector_nullarg

if NOT_STRICTMPI
noinst_PROGRAMS += \
                  type_extent_nullarg \
                  type_lb_nullarg \
                  type_ub_nullarg
endif


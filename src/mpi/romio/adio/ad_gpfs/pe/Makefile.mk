## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_AD_PE

AM_CPPFLAGS += -DBGL_OPTIM_STEP1_2=1 -DBGL_OPTIM_STEP1_1=1

noinst_HEADERS +=                                                    \
    adio/ad_gpfs/pe/ad_pe_aggrs.h

romio_other_sources +=                                               \
    adio/ad_gpfs/pe/ad_pe_aggrs.c                                    \
    adio/ad_gpfs/pe/ad_pe_hints.c

endif BUILD_AD_PE

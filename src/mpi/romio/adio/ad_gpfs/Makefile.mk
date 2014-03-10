## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_AD_GPFS

AM_CPPFLAGS += -DBGL_OPTIM_STEP1_2=1 -DBGL_OPTIM_STEP1_1=1

noinst_HEADERS +=                                                    \
    adio/ad_gpfs/bg/ad_bg_aggrs.h                                         \
    adio/ad_gpfs/ad_gpfs_aggrs.h                                         \
    adio/ad_gpfs/ad_gpfs.h                                               \
    adio/ad_gpfs/bg/ad_bg_pset.h                                          \
    adio/ad_gpfs/ad_gpfs_tuning.h

romio_other_sources +=                                               \
    adio/ad_gpfs/bg/ad_bg_aggrs.c                                         \
    adio/ad_gpfs/ad_gpfs_aggrs.c                                         \
    adio/ad_gpfs/ad_gpfs_close.c                                         \
    adio/ad_gpfs/ad_gpfs_flush.c                                         \
    adio/ad_gpfs/bg/ad_bg_hints.c                                         \
    adio/ad_gpfs/bg/ad_bg_pset.c                                          \
    adio/ad_gpfs/ad_gpfs_tuning.c                                        \
    adio/ad_gpfs/ad_gpfs.c                                               \
    adio/ad_gpfs/ad_gpfs_getsh.c                                         \
    adio/ad_gpfs/ad_gpfs_open.c                                          \
    adio/ad_gpfs/ad_gpfs_rdcoll.c                                        \
    adio/ad_gpfs/ad_gpfs_setsh.c                                         \
    adio/ad_gpfs/ad_gpfs_wrcoll.c

endif BUILD_AD_GPFS

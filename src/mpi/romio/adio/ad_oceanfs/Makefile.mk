##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory.
##

if BUILD_AD_OCEANFS

noinst_HEADERS += adio/ad_oceanfs/ad_oceanfs.h

romio_other_sources +=               \
    adio/ad_oceanfs/ad_oceanfs.c     \
    adio/ad_oceanfs/ad_oceanfs_open.c

endif BUILD_AD_OCEANFS

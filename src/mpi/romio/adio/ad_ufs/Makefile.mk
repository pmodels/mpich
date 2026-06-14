##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_AD_UFS

noinst_HEADERS += adio/ad_ufs/ad_ufs.h

romio_other_sources +=        \
    adio/ad_ufs/ad_ufs.c      \
    adio/ad_ufs/ad_ufs_open.c

endif BUILD_AD_UFS

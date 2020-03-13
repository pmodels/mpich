## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
##  (C) 2020 by Argonne National Laboratory.
##      See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2018-2020 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

if BUILD_AD_DAOS

noinst_HEADERS += adio/ad_daos/ad_daos.h

romio_other_sources += \
    adio/ad_daos/ad_daos.c \
    adio/ad_daos/ad_daos_close.c \
    adio/ad_daos/ad_daos_common.c \
    adio/ad_daos/ad_daos_fcntl.c \
    adio/ad_daos/ad_daos_features.c \
    adio/ad_daos/ad_daos_hhash.c \
    adio/ad_daos/ad_daos_hints.c \
    adio/ad_daos/ad_daos_io.c \
    adio/ad_daos/ad_daos_io_str.c \
    adio/ad_daos/ad_daos_open.c \
    adio/ad_daos/ad_daos_resize.c

endif BUILD_AD_DAOS


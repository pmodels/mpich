## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2018-2020 by Intel Corporation
##
## GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
## The Government's rights to use, modify, reproduce, release, perform, display,
## or disclose this software are subject to the terms of the Apache License as
## provided in Contract No. 8F-30005.
## Any reproduction of computer software, computer software documentation, or
## portions thereof marked with this legend must also reproduce the markings.
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


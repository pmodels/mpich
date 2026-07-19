##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/lib

libhydra_la_SOURCES += \
    lib/pmiserv_common.c

include lib/utils/Makefile.mk
include lib/tools/Makefile.mk


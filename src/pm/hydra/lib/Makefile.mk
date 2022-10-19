##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib

libhydra_la_SOURCES += \
    lib/pmiserv_common.c

include lib/utils/Makefile.mk
include lib/tools/Makefile.mk


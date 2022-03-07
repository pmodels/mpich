##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/bootstrap/utils

noinst_HEADERS += lib/tools/bootstrap/utils/bscu.h

libhydra_la_SOURCES += \
    lib/tools/bootstrap/utils/bscu_wait.c \
    lib/tools/bootstrap/utils/bscu_cb.c

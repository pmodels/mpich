##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/bootstrap/utils

noinst_HEADERS += lib/tools/bootstrap/utils/bscu.h

libhydra_la_SOURCES += \
    lib/tools/bootstrap/utils/bscu_wait.c \
    lib/tools/bootstrap/utils/bscu_cb.c

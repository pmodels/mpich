##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/bootstrap/persist

libhydra_la_SOURCES += lib/tools/bootstrap/persist/persist_init.c \
    lib/tools/bootstrap/persist/persist_launch.c \
    lib/tools/bootstrap/persist/persist_wait.c

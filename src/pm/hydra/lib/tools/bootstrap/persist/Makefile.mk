##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/lib/tools/bootstrap/persist

libhydra_la_SOURCES += lib/tools/bootstrap/persist/persist_init.c \
    lib/tools/bootstrap/persist/persist_launch.c \
    lib/tools/bootstrap/persist/persist_wait.c

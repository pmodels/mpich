##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/bootstrap/persist

libhydra_la_SOURCES += tools/bootstrap/persist/persist_init.c \
    tools/bootstrap/persist/persist_launch.c \
    tools/bootstrap/persist/persist_wait.c

## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

bin_PROGRAMS += hydra_persist

hydra_persist_SOURCES = tools/bootstrap/persist/persist_server.c
hydra_persist_CFLAGS = $(AM_CFLAGS)
hydra_persist_LDFLAGS = $(external_ldflags) -L$(top_builddir)
hydra_persist_LDADD = -lhydra $(external_libs)
hydra_persist_DEPENDENCIES = libhydra.la

noinst_HEADERS +=                            \
    tools/bootstrap/persist/persist.h        \
    tools/bootstrap/persist/persist_client.h \
    tools/bootstrap/persist/persist_server.h

libhydra_la_SOURCES += tools/bootstrap/persist/persist_init.c \
	tools/bootstrap/persist/persist_launch.c \
	tools/bootstrap/persist/persist_wait.c

doc1_src_txt += tools/bootstrap/persist/hydra_persist.txt

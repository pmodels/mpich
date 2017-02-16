## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/bstrap/ssh

noinst_HEADERS +=                     \
    libhydra/bstrap/ssh/hydra_bstrap_ssh.h \
    libhydra/bstrap/ssh/ssh_internal.h

libhydra_la_SOURCES += \
	libhydra/bstrap/ssh/ssh_internal.c \
	libhydra/bstrap/ssh/ssh_launch.c \
	libhydra/bstrap/ssh/ssh_finalize.c

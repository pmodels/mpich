## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/sock

noinst_HEADERS += libhydra/sock/hydra_sock.h

libhydra_la_SOURCES += \
	libhydra/sock/hydra_sock.c

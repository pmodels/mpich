## -*- Mode: Makefile; -*-
##
## (C) 2009 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/tools/bootstrap/utils

noinst_HEADERS += tools/bootstrap/utils/bscu.h

libhydra_la_SOURCES += \
	tools/bootstrap/utils/bscu_wait.c \
	tools/bootstrap/utils/bscu_cb.c

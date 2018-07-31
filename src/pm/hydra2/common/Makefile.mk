## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/common

noinst_LTLIBRARIES += libmpx.la
libmpx_la_SOURCES = \
		common/mpx_bcast.c \
		common/mpx_env.c

noinst_HEADERS += common/mpx.h

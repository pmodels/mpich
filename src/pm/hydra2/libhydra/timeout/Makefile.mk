##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/timeout

noinst_HEADERS += libhydra/timeout/hydra_timeout.h

libhydra_la_SOURCES += \
	libhydra/timeout/hydra_timeout.c

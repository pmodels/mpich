## -*- Mode: Makefile; -*-
##
## (C) 2008 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/pm/include

noinst_HEADERS += pm/include/pmci.h

noinst_LTLIBRARIES += libpm.la
libpm_la_SOURCES =

if hydra_pm_pmiserv
include pm/pmiserv/Makefile.mk
endif

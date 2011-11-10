## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtestthread.o from mtestthread.c
EXTRA_LIBRARIES = libmtestthread_dummy.a
libmtestthread_dummy_a_SOURCES = mtestthread.c
all-local: mtestthread.$(OBJEXT)


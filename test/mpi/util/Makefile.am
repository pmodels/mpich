# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS = -I$(srcdir)/../include -I../include -I$(srcdir)/../dtpools/include

all-local: mtest.$(OBJEXT) dtypes.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c mtest_common.c

#---- CXX ---------------------------------------------------------------------
all-local: mtest_cxx.$(OBJEXT)

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtest_cxx.o from mtest_cxx.cxx
EXTRA_LIBRARIES = libmtest_cxx.a
libmtest_cxx_a_SOURCES = mtest_cxx.cxx

#---- F77 ---------------------------------------------------------------------
all-local: mtest_f77.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_f77.a
libmtest_f77_a_SOURCES = mtest_f77.f

#---- F08 ---------------------------------------------------------------------
all-local: mtest_f08.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_f08.a
libmtest_f08_a_SOURCES = mtest_f08.f90

#---- thread ---------------------------------------------------------------------
all-local: mtest_thread.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_thread.a
libmtest_thread_a_SOURCES = mtest_thread.c

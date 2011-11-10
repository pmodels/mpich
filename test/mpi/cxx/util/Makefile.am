# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

## the simplemake version also included the following bits that seem wrong:
## -I../../../src/include -I${top_srcdir}/src/include
AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtest.o from mtest.cxx
EXTRA_LIBRARIES = libmtest_dummy.a
libmtest_dummy_a_SOURCES = mtest.cxx

all-local: mtest.$(OBJEXT)


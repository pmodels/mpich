## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtestf.o from mtestf.f
EXTRA_LIBRARIES = libmtest_dummy.a
libmtest_dummy_a_SOURCES = mtestf.f

all-local: mtestf.$(OBJEXT)


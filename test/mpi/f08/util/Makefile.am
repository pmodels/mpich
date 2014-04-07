## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2014 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtestf08.o from mtestf08.f90
EXTRA_LIBRARIES = libmtest_dummy.a
libmtest_dummy_a_SOURCES = mtestf08.f90

all-local: mtestf08.$(OBJEXT)

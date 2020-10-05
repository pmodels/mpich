##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS = -I$(srcdir)/../include -I../include -I$(srcdir)/../dtpools/include @cuda_CPPFLAGS@ @ze_CPPFLAGS@

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c mtest_common.c

EXTRA_LIBRARIES =
all_objs = mtest.$(OBJEXT) dtypes.$(OBJEXT)

#---- CXX ---------------------------------------------------------------------
if HAS_CXX

all_objs += mtest_cxx.$(OBJEXT)

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtest_cxx.o from mtest_cxx.cxx
EXTRA_LIBRARIES += libmtest_cxx.a
libmtest_cxx_a_SOURCES = mtest_cxx.cxx

endif

#---- F77 ---------------------------------------------------------------------
if HAS_F77

all_objs += mtest_f77.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_f77.a
libmtest_f77_a_SOURCES = mtest_f77.f

endif

#---- F08 ---------------------------------------------------------------------
if HAS_F08

all_objs += mtest_f08.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_f08.a
libmtest_f08_a_SOURCES = mtest_f08.f90

endif

#---- single ---------------------------------------------------------------------
all_objs += mtest_single.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_single.a
libmtest_single_a_SOURCES = mtest_single.c

#---- thread ---------------------------------------------------------------------
all_objs += mtest_thread.$(OBJEXT)

## another dummy library
EXTRA_LIBRARIES += libmtest_thread.a
libmtest_thread_a_SOURCES = mtest_thread.c

#---- all-local target -----------------------------------------------------------

all-local: $(all_objs)

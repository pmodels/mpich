# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist.in

# allocmemf is an "extra" program because it requires a Fortran extension
EXTRA_PROGRAMS = allocmemf

noinst_PROGRAMS = c2f2cf ctypesinf c2fmult

allocmemf_SOURCES = allocmemf.f
c2f2cf_SOURCES = c2f2c.c c2f2cf.f
ctypesinf_SOURCES = ctypesinf.f ctypesfromc.c

# C programs get a different mtest utility object
c2fmult_LDADD = $(top_builddir)/util/mtest.o
c2fmult_SOURCES = c2fmult.c

## add1size.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when add1size.h is updated
allocmemf.$(OBJEXT): add1size.h

# we don't get this from Makefile_f77.mtest and we don't include Makefile.mtest
$(top_builddir)/util/mtest.o:
	(cd $(top_builddir)/util && $(MAKE) mtest.o)


# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist.in

# avoid having to write many "foo_SOURCES = foo.f" lines
AM_DEFAULT_SOURCE_EXT = .f

noinst_PROGRAMS = namepubf spawnf spawnargvf spawnmultf spawnmult2f connaccf

## type1aint.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when type1aint.h is updated
spawnmult2f.$(OBJEXT): type1aint.h

# add "-lU77" (or equivalent) to the link line for these tests that use getarg
spawnargvf_LDADD = $(LDADD) $(F77_GETARG_LIBS)
spawnmultf_LDADD = $(LDADD) $(F77_GETARG_LIBS)


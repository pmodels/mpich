# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f" lines
AM_DEFAULT_SOURCE_EXT = .f

noinst_PROGRAMS = allpairf statusesf greqf mprobef

greqf_SOURCES = greqf.f dummyf.f

## attr1aints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when typeaints.h is updated
greqf.$(OBJEXT): attr1aints.h


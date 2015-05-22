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

noinst_PROGRAMS = typenamef typesnamef typecntsf typesubf typem2f gaddressf \
                  packef allctypesf hindex1f hindexed_blockf typename3f bottom

bottom_SOURCES=bottomc.c bottomf.f

## typeaints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

## ensure that dependent tests will be rebuilt when typeaints.h is updated
gaddressf.$(OBJEXT): typeaints.h
packef.$(OBJEXT): typeaints.h
typecntsf.$(OBJEXT): typeaints.h
typem2f.$(OBJEXT): typeaints.h
hindexed_blockf.$(OBJEXT): typeaints.h

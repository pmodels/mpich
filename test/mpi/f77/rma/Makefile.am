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

noinst_PROGRAMS = \
    winscale1f    \
    winscale2f    \
    winfencef     \
    wingetf       \
    winerrf       \
    winnamef      \
    wingroupf     \
    winaccf       \
    winaccf       \
    c2f2cwinf     \
    baseattrwinf  \
    winattrf      \
    winattr2f

if BUILD_MPIX_TESTS
noinst_PROGRAMS += aintf
endif

## this test was commented out in the simplemake version...
##    allocmem

c2f2cwinf_SOURCES = c2f2cwin.c c2f2cwinf.f

# see include note in Makefile_f77.mtest
AM_FFLAGS += -I../attr

## addsize.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when addsize.h is updated
baseattrwinf.$(OBJEXT): addsize.h
c2f2cwinf.$(OBJEXT): addsize.h
winaccf.$(OBJEXT): addsize.h
winerrf.$(OBJEXT): addsize.h
winfencef.$(OBJEXT): addsize.h
wingetf.$(OBJEXT): addsize.h
wingroupf.$(OBJEXT): addsize.h
winnamef.$(OBJEXT): addsize.h
winscale1f.$(OBJEXT): addsize.h
winscale2f.$(OBJEXT): addsize.h

## DJG: the following comment+code was present in the simplemake version... I'm
## not sure how accurate it is.
# bindir may be used in MPIEXEC, so make sure it is defined
BINDIR=${bindir}


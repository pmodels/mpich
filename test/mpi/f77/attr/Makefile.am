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

noinst_PROGRAMS =                   \
    attrmpi1f                       \
    baseattrf baseattr2f            \
    commattrf commattr2f            \
    commattr3f commattr4f           \
    typeattrf typeattr2f typeattr3f

## attraints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that "{comm,type}attr*f.o" will be rebuilt when attraints.h is updated
commattrf.$(OBJEXT): attraints.h
commattr2f.$(OBJEXT): attraints.h
commattr3f.$(OBJEXT): attraints.h
typeattrf.$(OBJEXT): attraints.h
typeattr2f.$(OBJEXT): attraints.h
typeattr3f.$(OBJEXT): attraints.h


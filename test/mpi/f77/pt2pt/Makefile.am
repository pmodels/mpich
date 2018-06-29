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

noinst_PROGRAMS = sendf \
                  ssendf \
                  rsendf \
                  isendf \
                  irsendf \
                  issendf \
                  psendf \
                  prsendf \
                  pssendf \
                  sendrecvf \
                  sendrecvreplf \
                  statusesf greqf \
                  mprobef

sendf_SOURCES = sendf.f utilsf.f
ssendf_SOURCES = ssendf.f utilsf.f
rsendf_SOURCES = rsendf.f utilsf.f
isendf_SOURCES = isendf.f utilsf.f
irsendf_SOURCES = irsendf.f utilsf.f
issendf_SOURCES = issendf.f utilsf.f
psendf_SOURCES = psendf.f utilsf.f
prsendf_SOURCES = prsendf.f utilsf.f
pssendf_SOURCES = pssendf.f utilsf.f
sendrecvf_SOURCES = sendrecvf.f utilsf.f
sendrecvreplf_SOURCES = sendrecvreplf.f utilsf.f
greqf_SOURCES = greqf.f dummyf.f

## attr1aints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when typeaints.h is updated
greqf.$(OBJEXT): attr1aints.h

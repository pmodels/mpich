# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = greqf08 \
                  statusesf08 \
                  mprobef08 \
                  sendf08 \
                  ssendf08 \
                  rsendf08 \
                  isendf08 \
                  irsendf08 \
                  issendf08 \
                  psendf08 \
                  prsendf08 \
                  pssendf08 \
                  sendrecvf08 \
                  sendrecvreplf08

sendf08_SOURCES = sendf08.f90 utilsf08.f90
ssendf08_SOURCES = ssendf08.f90 utilsf08.f90
rsendf08_SOURCES = rsendf08.f90 utilsf08.f90
isendf08_SOURCES = isendf08.f90 utilsf08.f90
irsendf08_SOURCES = irsendf08.f90 utilsf08.f90
issendf08_SOURCES = issendf08.f90 utilsf08.f90
psendf08_SOURCES = psendf08.f90 utilsf08.f90
prsendf08_SOURCES = prsendf08.f90 utilsf08.f90
pssendf08_SOURCES = pssendf08.f90 utilsf08.f90
sendrecvf08_SOURCES = sendrecvf08.f90 utilsf08.f90
sendrecvreplf08_SOURCES = sendrecvreplf08.f90 utilsf08.f90
greqf08_SOURCES = greqf08.f90 dummyf08.f90

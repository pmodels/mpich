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
                  allpair_sendf08 \
                  allpair_ssendf08 \
                  allpair_rsendf08 \
                  allpair_isendf08 \
                  allpair_irsendf08 \
                  allpair_issendf08 \
                  allpair_psendf08 \
                  allpair_prsendf08 \
                  allpair_pssendf08 \
                  allpair_sendrecvf08 \
                  allpair_sendrecvreplf08

allpair_sendf08_SOURCES = allpair_sendf08.f90 allpair_utilsf08.f90
allpair_ssendf08_SOURCES = allpair_ssendf08.f90 allpair_utilsf08.f90
allpair_rsendf08_SOURCES = allpair_rsendf08.f90 allpair_utilsf08.f90
allpair_isendf08_SOURCES = allpair_isendf08.f90 allpair_utilsf08.f90
allpair_irsendf08_SOURCES = allpair_irsendf08.f90 allpair_utilsf08.f90
allpair_issendf08_SOURCES = allpair_issendf08.f90 allpair_utilsf08.f90
allpair_psendf08_SOURCES = allpair_psendf08.f90 allpair_utilsf08.f90
allpair_prsendf08_SOURCES = allpair_prsendf08.f90 allpair_utilsf08.f90
allpair_pssendf08_SOURCES = allpair_pssendf08.f90 allpair_utilsf08.f90
allpair_sendrecvf08_SOURCES = allpair_sendrecvf08.f90 allpair_utilsf08.f90
allpair_sendrecvreplf08_SOURCES = allpair_sendrecvreplf08.f90 allpair_utilsf08.f90
greqf08_SOURCES = greqf08.f90 dummyf08.f90

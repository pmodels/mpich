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

noinst_PROGRAMS = allpair_sendf \
                  allpair_ssendf \
                  allpair_rsendf \
                  allpair_isendf \
                  allpair_irsendf \
                  allpair_issendf \
                  allpair_psendf \
                  allpair_prsendf \
                  allpair_pssendf \
                  allpair_sendrecvf \
                  allpair_sendrecvreplf \
                  statusesf greqf \
                  mprobef

allpair_sendf_SOURCES = allpair_sendf.f allpair_utilsf.f
allpair_ssendf_SOURCES = allpair_ssendf.f allpair_utilsf.f
allpair_rsendf_SOURCES = allpair_rsendf.f allpair_utilsf.f
allpair_isendf_SOURCES = allpair_isendf.f allpair_utilsf.f
allpair_irsendf_SOURCES = allpair_irsendf.f allpair_utilsf.f
allpair_issendf_SOURCES = allpair_issendf.f allpair_utilsf.f
allpair_psendf_SOURCES = allpair_psendf.f allpair_utilsf.f
allpair_prsendf_SOURCES = allpair_prsendf.f allpair_utilsf.f
allpair_pssendf_SOURCES = allpair_pssendf.f allpair_utilsf.f
allpair_sendrecvf_SOURCES = allpair_sendrecvf.f allpair_utilsf.f
allpair_sendrecvreplf_SOURCES = allpair_sendrecvreplf.f allpair_utilsf.f
greqf_SOURCES = greqf.f dummyf.f

## attr1aints.h will be distributed because it's listed in AC_CONFIG_FILES/AC_OUTPUT

# ensure that dependent tests will be rebuilt when typeaints.h is updated
greqf.$(OBJEXT): attr1aints.h


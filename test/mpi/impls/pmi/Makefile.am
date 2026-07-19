##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

LDADD = -lpmi

testing:
	$(top_srcdir)/runtests -srcdir=$(srcdir) -tests=$(TESTLIST) -testdirs=$(TESTDIRS) \
	    -mpiexec="${MPIEXEC}" $(RUNTESTS_OPTS) -xmlfile=$(SUMMARY_BASENAME).xml

noinst_PROGRAMS = \
    test_pmi1 \
    test_pmi2

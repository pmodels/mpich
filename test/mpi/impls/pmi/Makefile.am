##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

LDADD = -lpmi

testing:
	$(top_srcdir)/runtests -srcdir=$(srcdir) -tests=$(TESTLIST) -testdirs=$(TESTDIRS) \
	    -mpiexec="${MPIEXEC}" $(RUNTESTS_OPTS) -xmlfile=$(SUMMARY_BASENAME).xml

noinst_PROGRAMS = \
    test_pmi1 \
    test_pmi2

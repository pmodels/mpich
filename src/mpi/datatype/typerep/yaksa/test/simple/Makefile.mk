##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

testlists += $(top_srcdir)/test/simple/testlist.gen
EXTRA_DIST += $(top_srcdir)/test/simple/testlist.gen

EXTRA_PROGRAMS += \
	test/simple/simple_test \
        test/simple/lbub \
	test/simple/test_contig \
	test/simple/threaded_test

test_simple_simple_test_CPPFLAGS = $(test_cppflags)
test_simple_lbub_CPPFLAGS = $(test_cppflags)
test_simple_test_contig_CPPFLAGS = $(test_cppflags)
test_simple_threaded_test_CPPFLAGS = $(test_cppflags)

test-simple:
	@$(top_srcdir)/test/runtests.py --summary=$(top_builddir)/test/simple/summary.junit.xml \
                test/simple/testlist.gen

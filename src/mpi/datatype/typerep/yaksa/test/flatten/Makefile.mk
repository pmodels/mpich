##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

flatten_testlists = $(top_srcdir)/test/flatten/testlist.gen \
	$(top_srcdir)/test/flatten/testlist.threads.gen

testlists += $(flatten_testlists)
EXTRA_DIST += $(flatten_testlists)

EXTRA_PROGRAMS += \
	test/flatten/flatten

test_flatten_flatten_CPPFLAGS = $(test_cppflags)

test-flatten:
	@$(top_srcdir)/test/runtests.py --summary=$(top_builddir)/test/flatten/summary.junit.xml \
                $(flatten_testlists)

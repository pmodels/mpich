##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

iov_testlists = $(top_srcdir)/test/iov/testlist.gen \
	$(top_srcdir)/test/iov/testlist.threads.gen

testlists += $(iov_testlists)
EXTRA_DIST += $(iov_testlists)

EXTRA_PROGRAMS += \
	test/iov/iov

test_iov_iov_CPPFLAGS = $(test_cppflags)

test-iov:
	@$(top_srcdir)/test/runtests.py --summary=$(top_builddir)/test/iov/summary.junit.xml \
                $(iov_testlists)

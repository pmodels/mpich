#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += tests/ctree/ctree_test        \
                  tests/ctree/ctree_test_rand   \
                  tests/ctree/ctree_test_rand_interval

TESTS          += tests/ctree/ctree_test        \
                  tests/ctree/ctree_test_rand   \
                  tests/ctree/ctree_test_rand_interval


tests_ctree_ctree_test_SOURCES = $(top_srcdir)/tests/ctree/ctree_test.c
tests_ctree_ctree_test_LDADD = -larmci -lm
tests_ctree_ctree_test_DEPENDENCIES = libarmci.la

tests_ctree_ctree_test_rand_SOURCES = $(top_srcdir)/tests/ctree/ctree_test_rand.c
tests_ctree_ctree_test_rand_LDADD = -larmci -lm
tests_ctree_ctree_test_rand_DEPENDENCIES = libarmci.la

tests_ctree_ctree_test_rand_interval_SOURCES = $(top_srcdir)/tests/ctree/ctree_test_rand_interval.c
tests_ctree_ctree_test_rand_interval_LDADD = -larmci -lm
tests_ctree_ctree_test_rand_interval_DEPENDENCIES = libarmci.la

#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += tests/ctree/ctree_test        \
                  tests/ctree/ctree_test_rand   \
                  tests/ctree/ctree_test_rand_interval

TESTS          += tests/ctree/ctree_test        \
                  tests/ctree/ctree_test_rand   \
                  tests/ctree/ctree_test_rand_interval


tests_ctree_ctree_test_LDADD = libarmci.la -lm
tests_ctree_ctree_test_rand_LDADD = libarmci.la -lm
tests_ctree_ctree_test_rand_interval_LDADD = libarmci.la -lm

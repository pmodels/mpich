#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += tests/test_onesided         \
                  tests/test_onesided_shared  \
                  tests/test_onesided_shared_dla \
                  tests/test_mutex            \
                  tests/test_mutex_rmw        \
                  tests/test_mutex_trylock    \
                  tests/test_malloc           \
                  tests/test_malloc_irreg     \
                  tests/ARMCI_PutS_latency    \
                  tests/ARMCI_AccS_latency    \
                  tests/test_groups           \
                  tests/test_group_split      \
                  tests/test_malloc_group     \
                  tests/test_accs             \
                  tests/test_accs_dla         \
                  tests/test_puts             \
                  tests/test_puts_gets        \
                  tests/test_puts_gets_dla    \
                  tests/test_assert           \
                  tests/test_igop             \
                  # end

TESTS          += tests/test_onesided         \
                  tests/test_onesided_shared  \
                  tests/test_onesided_shared_dla \
                  tests/test_mutex            \
                  tests/test_mutex_rmw        \
                  tests/test_mutex_trylock    \
                  tests/test_malloc           \
                  tests/test_malloc_irreg     \
                  tests/ARMCI_PutS_latency    \
                  tests/ARMCI_AccS_latency    \
                  tests/test_groups           \
                  tests/test_group_split      \
                  tests/test_malloc_group     \
                  tests/test_accs             \
                  tests/test_accs_dla         \
                  tests/test_puts             \
                  tests/test_puts_gets        \
                  tests/test_puts_gets_dla    \
                  tests/test_igop             \
                  # end

XFAIL_TESTS    += tests/test_assert           \
                  # end

tests_test_onesided_LDADD = libarmci.la
tests_test_onesided_shared_LDADD = libarmci.la
tests_test_onesided_shared_dla_LDADD = libarmci.la
tests_test_mutex_LDADD = libarmci.la
tests_test_mutex_rmw_LDADD = libarmci.la
tests_test_mutex_trylock_LDADD = libarmci.la
tests_test_malloc_LDADD = libarmci.la
tests_test_malloc_irreg_LDADD = libarmci.la
tests_ARMCI_PutS_latency_LDADD = libarmci.la
tests_ARMCI_AccS_latency_LDADD = libarmci.la
tests_test_groups_LDADD = libarmci.la
tests_test_group_split_LDADD = libarmci.la
tests_test_malloc_group_LDADD = libarmci.la
tests_test_accs_LDADD = libarmci.la
tests_test_accs_dla_LDADD = libarmci.la
tests_test_puts_LDADD = libarmci.la
tests_test_puts_gets_LDADD = libarmci.la
tests_test_puts_gets_dla_LDADD = libarmci.la
tests_test_assert_LDADD = libarmci.la
tests_test_igop_LDADD = libarmci.la

include tests/mpi/Makefile.mk
include tests/ctree/Makefile.mk

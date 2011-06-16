#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

include tests/mpi/Makefile.mk

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

tests_test_onesided_SOURCES = $(top_srcdir)/tests/test_onesided.c
tests_test_onesided_LDADD = -larmci
tests_test_onesided_DEPENDENCIES = libarmci.la

tests_test_onesided_shared_SOURCES = $(top_srcdir)/tests/test_onesided_shared.c
tests_test_onesided_shared_LDADD = -larmci
tests_test_onesided_shared_DEPENDENCIES = libarmci.la

tests_test_onesided_shared_dla_SOURCES = $(top_srcdir)/tests/test_onesided_shared_dla.c
tests_test_onesided_shared_dla_LDADD = -larmci
tests_test_onesided_shared_dla_DEPENDENCIES = libarmci.la

tests_test_mutex_SOURCES = $(top_srcdir)/tests/test_mutex.c
tests_test_mutex_LDADD = -larmci
tests_test_mutex_DEPENDENCIES = libarmci.la

tests_test_mutex_rmw_SOURCES = $(top_srcdir)/tests/test_mutex_rmw.c
tests_test_mutex_rmw_LDADD = -larmci
tests_test_mutex_rmw_DEPENDENCIES = libarmci.la

tests_test_mutex_trylock_SOURCES = $(top_srcdir)/tests/test_mutex_trylock.c
tests_test_mutex_trylock_LDADD = -larmci
tests_test_mutex_trylock_DEPENDENCIES = libarmci.la

tests_test_malloc_SOURCES = $(top_srcdir)/tests/test_malloc.c
tests_test_malloc_LDADD = -larmci
tests_test_malloc_DEPENDENCIES = libarmci.la

tests_test_malloc_irreg_SOURCES = $(top_srcdir)/tests/test_malloc_irreg.c
tests_test_malloc_irreg_LDADD = -larmci
tests_test_malloc_irreg_DEPENDENCIES = libarmci.la

tests_ARMCI_PutS_latency_SOURCES = $(top_srcdir)/tests/ARMCI_PutS_latency.c
tests_ARMCI_PutS_latency_LDADD = -larmci
tests_ARMCI_PutS_latency_DEPENDENCIES = libarmci.la

tests_ARMCI_AccS_latency_SOURCES = $(top_srcdir)/tests/ARMCI_AccS_latency.c
tests_ARMCI_AccS_latency_LDADD = -larmci
tests_ARMCI_AccS_latency_DEPENDENCIES = libarmci.la

tests_test_groups_SOURCES = $(top_srcdir)/tests/test_groups.c
tests_test_groups_LDADD = -larmci
tests_test_groups_DEPENDENCIES = libarmci.la
tests_test_group_split_LDADD = -larmci
tests_test_group_split_DEPENDENCIES = libarmci.la

tests_test_malloc_group_SOURCES = $(top_srcdir)/tests/test_malloc_group.c
tests_test_malloc_group_LDADD = -larmci
tests_test_malloc_group_DEPENDENCIES = libarmci.la

tests_test_accs_SOURCES = $(top_srcdir)/tests/test_accs.c
tests_test_accs_LDADD = -larmci
tests_test_accs_DEPENDENCIES = libarmci.la

tests_test_accs_dla_SOURCES = $(top_srcdir)/tests/test_accs_dla.c
tests_test_accs_dla_LDADD = -larmci
tests_test_accs_dla_DEPENDENCIES = libarmci.la

tests_test_puts_SOURCES = $(top_srcdir)/tests/test_puts.c
tests_test_puts_LDADD = -larmci
tests_test_puts_DEPENDENCIES = libarmci.la

tests_test_puts_gets_SOURCES = $(top_srcdir)/tests/test_puts_gets.c
tests_test_puts_gets_LDADD = -larmci
tests_test_puts_gets_DEPENDENCIES = libarmci.la

tests_test_puts_gets_dla_SOURCES = $(top_srcdir)/tests/test_puts_gets_dla.c
tests_test_puts_gets_dla_LDADD = -larmci
tests_test_puts_gets_dla_DEPENDENCIES = libarmci.la

tests_test_assert_SOURCES = $(top_srcdir)/tests/test_assert.c
tests_test_assert_LDADD = -larmci
tests_test_assert_DEPENDENCIES = libarmci.la

tests_test_igop_SOURCES = $(top_srcdir)/tests/test_igop.c
tests_test_igop_LDADD = -larmci
tests_test_igop_DEPENDENCIES = libarmci.la

include tests/ctree/Makefile.mk

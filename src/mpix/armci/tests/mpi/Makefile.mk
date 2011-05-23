#
# Copyright (C) 2010. See COPYRIGHT in top-level directory.
#

check_PROGRAMS += \
                  tests/mpi/ping-pong-mpi               \
                  tests/mpi/test_mpi_accs               \
                  tests/mpi/test_mpi_indexed_accs       \
                  tests/mpi/test_mpi_indexed_gets       \
                  tests/mpi/test_mpi_indexed_puts_gets  \
                  tests/mpi/test_mpi_subarray_accs      \
                  tests/mpi/test_win_create             \
                  # end

TESTS          += \
                  tests/mpi/ping-pong-mpi      \
                  tests/mpi/test_mpi_accs               \
                  tests/mpi/test_mpi_indexed_accs       \
                  tests/mpi/test_mpi_indexed_gets       \
                  tests/mpi/test_mpi_indexed_puts_gets  \
                  tests/mpi/test_mpi_subarray_accs      \
                  tests/mpi/test_win_create             \
                  # end

tests_mpi_ping_pong_mpi_SOURCES = $(top_srcdir)/tests/mpi/ping-pong-mpi.c
tests_mpi_ping_pong_mpi_LDADD = -larmci
tests_mpi_ping_pong_mpi_DEPENDENCIES = libarmci.la

tests_mpi_test_mpi_accs = $(top_srcdir)/tests/mpi/test_mpi_accs.c
tests_mpi_test_mpi_accs_LDADD = -larmci
tests_mpi_test_mpi_accs_DEPENDENCIES = libarmci.la

tests_mpi_test_mpi_indexed_accs = $(top_srcdir)/tests/mpi/test_mpi_indexed_accs.c
tests_mpi_test_mpi_indexed_accs_LDADD = -larmci
tests_mpi_test_mpi_indexed_accs_DEPENDENCIES = libarmci.la

tests_mpi_test_mpi_indexed_gets = $(top_srcdir)/tests/mpi/test_mpi_indexed_gets.c
tests_mpi_test_mpi_indexed_gets_LDADD = -larmci
tests_mpi_test_mpi_indexed_gets_DEPENDENCIES = libarmci.la

tests_mpi_test_mpi_indexed_puts_gets = $(top_srcdir)/tests/mpi/test_mpi_indexed_puts_gets.c
tests_mpi_test_mpi_indexed_puts_gets_LDADD = -larmci
tests_mpi_test_mpi_indexed_puts_gets_DEPENDENCIES = libarmci.la

tests_mpi_test_mpi_subarray_accs = $(top_srcdir)/tests/mpi/test_mpi_subarray_accs.c
tests_mpi_test_mpi_subarray_accs_LDADD = -larmci
tests_mpi_test_mpi_subarray_accs_DEPENDENCIES = libarmci.la

tests_mpi_test_win_create = $(top_srcdir)/tests/mpi/test_win_create.c
tests_mpi_test_win_create_LDADD = -larmci
tests_mpi_test_win_create_DEPENDENCIES = libarmci.la

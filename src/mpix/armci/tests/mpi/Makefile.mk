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
                  tests/mpi/test_mpi_accs               \
                  tests/mpi/test_mpi_indexed_accs       \
                  tests/mpi/test_mpi_indexed_gets       \
                  tests/mpi/test_mpi_indexed_puts_gets  \
                  tests/mpi/test_mpi_subarray_accs      \
                  tests/mpi/test_win_create             \
                  #tests/mpi/ping-pong-mpi      \
                  # end

tests_mpi_ping_pong_mpi_LDADD = libarmci.la
tests_mpi_test_mpi_accs_LDADD = libarmci.la
tests_mpi_test_mpi_indexed_accs_LDADD = libarmci.la
tests_mpi_test_mpi_indexed_gets_LDADD = libarmci.la
tests_mpi_test_mpi_indexed_puts_gets_LDADD = libarmci.la
tests_mpi_test_mpi_subarray_accs_LDADD = libarmci.la
tests_mpi_test_win_create_LDADD = libarmci.la

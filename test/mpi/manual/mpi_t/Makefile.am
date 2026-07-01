##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

include $(top_srcdir)/Makefile_single.mtest

# these tests are specific to the MPICH ch3:nemesis implementation of nemesis
nemesis_tests = nem_fbox_fallback_to_queue_count \
                unexpected_recvq_buffer_size

noinst_PROGRAMS = mpit_test mpit_test2 \
                  $(nemesis_tests)

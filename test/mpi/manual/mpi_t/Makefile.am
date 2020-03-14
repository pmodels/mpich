##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

# these tests are specific to the MPICH ch3:nemesis implementation of nemesis
nemesis_tests = nem_fbox_fallback_to_queue_count \
                unexpected_recvq_buffer_size

noinst_PROGRAMS = mpit_test mpit_test2 \
                  $(nemesis_tests)

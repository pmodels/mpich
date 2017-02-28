# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

TESTS = \
	cs_thruput_tkt \
	cs_thruput_mcs \
	cs_thruput_csvmcs \
	cs_thruput_tlp

XFAIL_TESTS =

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)

include $(top_srcdir)/test/Makefile.mk

cs_thruput_tkt_SOURCES = cs_thruput.c
cs_thruput_mcs_SOURCES = cs_thruput.c
cs_thruput_csvmcs_SOURCES = cs_thruput.c
cs_thruput_tlp_SOURCES = cs_thruput.c

cs_thruput_tkt_CFLAGS = -DZMTEST_USE_TICKET -D_GNU_SOURCE
cs_thruput_mcs_CFLAGS = -DZMTEST_USE_MCS -D_GNU_SOURCE
cs_thruput_csvmcs_CFLAGS = -DZMTEST_USE_CSVMCS -D_GNU_SOURCE
cs_thruput_tlp_CFLAGS = -DZMTEST_USE_TLP -D_GNU_SOURCE

cs_thruput_tkt_LDFLAGS = -pthread
cs_thruput_mcs_LDFLAGS = -pthread
cs_thruput_csvmcs_LDFLAGS = -pthread
cs_thruput_tlp_LDFLAGS = -pthread

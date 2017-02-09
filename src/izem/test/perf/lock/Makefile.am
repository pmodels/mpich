# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

TESTS = \
	thread_scale_tlp

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)

include $(top_srcdir)/test/Makefile.mk

thread_scale_tlp_SOURCES = thread_scale_tlp.c

thread_scale_tlp_CFLAGS = -fopenmp

thread_scale_tlp_LDFLAGS = -fopenmp

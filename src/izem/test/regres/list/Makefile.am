# -*- Mode: Makefile; -*-
#
# See COPYRIGHT in top-level directory.
#

TESTS = \
	array_to_list

XFAIL_TESTS =

check_PROGRAMS = $(TESTS)
noinst_PROGRAMS = $(TESTS)

include $(top_srcdir)/test/Makefile.mk

array_to_list_SOURCES = array_to_list.c

array_to_list_CFLAGS =

array_to_list_LDFLAGS = -pthread

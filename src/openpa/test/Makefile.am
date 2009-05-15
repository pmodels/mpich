# -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#  
#  (C) 2008 by Argonne National Laboratory.
#      See COPYRIGHT in top-level directory.

TESTS = sanity test_primitives test_barriers

check_PROGRAMS = $(TESTS)

LDADD = ../src/libopa.a

sanity_SOURCES = sanity.c
test_primitives_SOURCES = test_primitives.c opa_test.h
test_barriers_SOURCES = test_barriers.c opa_test.h

AM_CPPFLAGS = -I$(top_srcdir)/src

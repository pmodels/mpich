# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS = -I$(srcdir)/../include -I../include

EXTRA_PROGRAMS = utest_example
utest_example_SOURCES = utest_example.c utest.c

all_objs = utest.$(OBJEXT)

all-local: $(all_objs)

# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS = -I$(srcdir)/../include -I../include -I$(srcdir)/../dtpools/include

mtest.$(OBJEXT): mtest.c
dtypes.$(OBJEXT): dtypes.c
all-local: mtest.$(OBJEXT) dtypes.$(OBJEXT)

EXTRA_PROGRAMS = mtestcheck dtypes
mtestcheck_SOURCES = mtestcheck.c mtest.c

## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/threads/Makefile_threads.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = multirma multiget multifence \
                  mt_put_huge mt_put_normal mt_get_huge mt_get_normal

#MT Put/Get tests with huge messages
mt_put_huge_SOURCES = mt_putget.c
mt_put_huge_CPPFLAGS = -DHUGE_COUNT -DPUT_TEST $(AM_CPPFLAGS)

mt_get_huge_SOURCES = mt_putget.c
mt_get_huge_CPPFLAGS = -DHUGE_COUNT -DGET_TEST $(AM_CPPFLAGS)

#MT Put/Get tests with small messages
mt_put_normal_SOURCES = mt_putget.c
mt_put_normal_CPPFLAGS = -DPUT_TEST $(AM_CPPFLAGS)

mt_get_normal_SOURCES = mt_putget.c
mt_get_normal_CPPFLAGS = -DGET_TEST $(AM_CPPFLAGS)

## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/threads/Makefile_threads.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = multirma multiget multifence \
                  mt_put_huge mt_put_normal mt_get_huge mt_get_normal \
                  mt_accfence_huge mt_accfence_normal \
                  mt_getaccfence_huge mt_getaccfence_normal mt_fetchandop mt_compareandswap \
                  mt_reqops

#MT tests with huge messages
mt_put_huge_SOURCES = mt_putget.c
mt_put_huge_CPPFLAGS = -DHUGE_COUNT -DPUT_TEST $(AM_CPPFLAGS)

mt_get_huge_SOURCES = mt_putget.c
mt_get_huge_CPPFLAGS = -DHUGE_COUNT -DGET_TEST $(AM_CPPFLAGS)

mt_accfence_huge_SOURCES = mt_accfence.c
mt_accfence_huge_CPPFLAGS = -DHUGE_COUNT $(AM_CPPFLAGS)

mt_getaccfence_huge_SOURCES = mt_getaccfence.c
mt_getaccfence_huge_CPPFLAGS = -DHUGE_COUNT $(AM_CPPFLAGS)

mt_reqops_SOURCES = mt_reqops.c
mt_reqops_CPPFLAGS = -DHUGE_COUNT $(AM_CPPFLAGS)

#MT tests with small messages
mt_put_normal_SOURCES = mt_putget.c
mt_put_normal_CPPFLAGS = -DPUT_TEST $(AM_CPPFLAGS)

mt_get_normal_SOURCES = mt_putget.c
mt_get_normal_CPPFLAGS = -DGET_TEST $(AM_CPPFLAGS)

mt_accfence_normal_SOURCES = mt_accfence.c
mt_accfence_normal_CPPFLAGS = $(AM_CPPFLAGS)

mt_getaccfence_normal_SOURCES = mt_getaccfence.c
mt_getaccfence_normal_CPPFLAGS = $(AM_CPPFLAGS)

mt_fetchandop_SOURCES = mt_fetchandop.c
mt_fetchandop_CPPFLAGS = $(AM_CPPFLAGS)

mt_compareandswap_SOURCES = mt_compareandswap.c
mt_compareandswap_CPPFLAGS = $(AM_CPPFLAGS)

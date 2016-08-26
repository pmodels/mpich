## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/include

noinst_HEADERS +=                          \
    src/mpid/include/mpidu_pre.h


include $(top_srcdir)/src/mpid/ch3/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/Makefile.mk
include $(top_srcdir)/src/mpid/common/Makefile.mk

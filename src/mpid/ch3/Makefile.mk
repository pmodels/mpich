## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

errnames_txt_files += src/mpid/ch3/errnames.txt

# note that the includes always happen but the effects of their contents are
# affected by "if BUILD_CH3"
if BUILD_CH3

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/include   \
               -I$(top_builddir)/src/mpid/ch3/include

noinst_HEADERS +=                      \
    src/mpid/ch3/include/mpidftb.h     \
    src/mpid/ch3/include/mpidimpl.h    \
    src/mpid/ch3/include/mpidpkt.h     \
    src/mpid/ch3/include/mpidpost.h    \
    src/mpid/ch3/include/mpidpre.h     \
    src/mpid/ch3/include/mpid_thread.h \
    src/mpid/ch3/include/mpidrma.h

include $(top_srcdir)/src/mpid/ch3/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/util/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/Makefile.mk

endif BUILD_CH3


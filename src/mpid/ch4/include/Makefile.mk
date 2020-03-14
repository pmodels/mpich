##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/include

noinst_HEADERS += src/mpid/ch4/include/netmodpre.h  \
                  src/mpid/ch4/include/shmpre.h     \
                  src/mpid/ch4/include/mpidch4.h    \
                  src/mpid/ch4/include/mpidch4r.h   \
                  src/mpid/ch4/include/mpidimpl.h   \
                  src/mpid/ch4/include/mpidpre.h    \
                  src/mpid/ch4/include/mpid_sched.h \
                  src/mpid/ch4/include/mpid_thread.h

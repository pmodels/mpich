##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/include \
               -I$(top_srcdir)/src/mpid/ch4/shm/posix

noinst_HEADERS += src/mpid/ch4/shm/include/shm.h

include $(top_srcdir)/src/mpid/ch4/shm/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/stubshm/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/ipc/Makefile.mk

##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/include \
               -I$(top_srcdir)/src/mpid/ch4/shm/posix

noinst_HEADERS += src/mpid/ch4/shm/include/shm.h

include $(top_srcdir)/src/mpid/ch4/shm/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/stubshm/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/ipc/Makefile.mk

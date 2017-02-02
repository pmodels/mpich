## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

if BUILD_SHM_POSIX

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/posix/eager/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/shm/posix/eager/include

noinst_HEADERS += src/mpid/ch4/shm/posix/eager/include/posix_eager.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_pre.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_transaction.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_impl.h

include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/fbox/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/iqueue/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/stub/Makefile.mk

endif

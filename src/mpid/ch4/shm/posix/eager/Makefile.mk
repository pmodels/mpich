##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_POSIX

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/posix/eager/include
AM_CPPFLAGS += -I$(top_builddir)/src/mpid/ch4/shm/posix/eager/include

noinst_HEADERS += src/mpid/ch4/shm/posix/eager/include/posix_eager.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_pre.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_transaction.h \
                  src/mpid/ch4/shm/posix/eager/include/posix_eager_impl.h

include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/iqueue/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/shm/posix/eager/stub/Makefile.mk

endif

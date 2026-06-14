##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

errnames_txt_files += src/mpid/ch3/util/sock/errnames.txt

if BUILD_CH3_UTIL_SOCK

mpi_core_sources +=               \
    src/mpid/ch3/util/sock/ch3u_init_sock.c     \
    src/mpid/ch3/util/sock/ch3u_connect_sock.c  \
    src/mpid/ch3/util/sock/ch3u_getinterfaces.c

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch3/util/sock   \
               -I$(top_builddir)/src/mpid/ch3/util/sock

noinst_HEADERS += src/mpid/ch3/util/sock/ch3usock.h

endif BUILD_CH3_UTIL_SOCK

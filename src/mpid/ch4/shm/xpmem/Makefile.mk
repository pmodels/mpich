## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

#if BUILD_SHM_XPMEM

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/xpmem

noinst_HEADERS += src/mpid/ch4/shm/xpmem/shm_xpmem.h      \
                  src/mpid/ch4/shm/xpmem/xpmem_win.h

#endif

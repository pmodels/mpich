## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/src

noinst_HEADERS += src/mpid/ch4/src/ch4_comm.h     \
                  src/mpid/ch4/src/ch4_init.h     \
                  src/mpid/ch4/src/ch4_progress.h \
                  src/mpid/ch4/src/ch4_request.h  \
                  src/mpid/ch4/src/ch4_send.h     \
                  src/mpid/ch4/src/ch4_types.h    \
                  src/mpid/ch4/src/ch4_impl.h     \
                  src/mpid/ch4/src/ch4_probe.h    \
                  src/mpid/ch4/src/ch4_proc.h     \
                  src/mpid/ch4/src/ch4_recv.h     \
                  src/mpid/ch4/src/ch4_rma.h      \
                  src/mpid/ch4/src/ch4_spawn.h    \
                  src/mpid/ch4/src/ch4_win.h      \
                  src/mpid/ch4/src/ch4r_probe.h   \
                  src/mpid/ch4/src/ch4r_rma.h     \
                  src/mpid/ch4/src/ch4r_win.h     \
                  src/mpid/ch4/src/ch4r_init.h    \
                  src/mpid/ch4/src/ch4r_proc.h    \
                  src/mpid/ch4/src/ch4i_comm.h    \
                  src/mpid/ch4/src/ch4r_recvq.h   \
                  src/mpid/ch4/src/ch4r_recv.h    \
                  src/mpid/ch4/src/ch4i_util.h 	  \
                  src/mpid/ch4/src/ch4r_symheap.h \
                  src/mpid/ch4/src/ch4r_buf.h     \
                  src/mpid/ch4/src/ch4r_request.h

mpi_core_sources += src/mpid/ch4/src/ch4_globals.c        \
                    src/mpid/ch4/src/mpid_ch4_net_array.c

if BUILD_CH4_COLL_TUNING
mpi_core_sources += src/mpid/ch4/src/ch4_coll_globals.c
else
mpi_core_sources += src/mpid/ch4/src/ch4_coll_globals_default.c
endif
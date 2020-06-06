##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/src

noinst_HEADERS += src/mpid/ch4/src/ch4_comm.h     \
                  src/mpid/ch4/src/ch4_request.h  \
                  src/mpid/ch4/src/ch4_send.h     \
                  src/mpid/ch4/src/ch4_types.h    \
                  src/mpid/ch4/src/ch4_impl.h     \
                  src/mpid/ch4/src/ch4_probe.h    \
                  src/mpid/ch4/src/ch4_proc.h     \
                  src/mpid/ch4/src/ch4_recv.h     \
                  src/mpid/ch4/src/ch4_rma.h      \
                  src/mpid/ch4/src/ch4_win.h      \
                  src/mpid/ch4/src/ch4_wait.h     \
                  src/mpid/ch4/src/ch4r_probe.h   \
                  src/mpid/ch4/src/ch4r_rma.h     \
                  src/mpid/ch4/src/ch4r_win.h     \
                  src/mpid/ch4/src/ch4r_init.h    \
                  src/mpid/ch4/src/ch4r_proc.h    \
                  src/mpid/ch4/src/ch4i_comm.h    \
                  src/mpid/ch4/src/ch4r_recvq.h   \
                  src/mpid/ch4/src/ch4r_recv.h    \
                  src/mpid/ch4/src/ch4r_callbacks.h     \
                  src/mpid/ch4/src/ch4r_rma_origin_callbacks.h     \
                  src/mpid/ch4/src/ch4r_rma_target_callbacks.h     \
                  src/mpid/ch4/src/ch4r_request.h

mpi_core_sources += src/mpid/ch4/src/ch4_globals.c        \
                    src/mpid/ch4/src/ch4_impl.c           \
                    src/mpid/ch4/src/ch4_init.c           \
                    src/mpid/ch4/src/ch4_comm.c           \
                    src/mpid/ch4/src/ch4_spawn.c          \
                    src/mpid/ch4/src/ch4_progress.c       \
                    src/mpid/ch4/src/ch4_win.c            \
                    src/mpid/ch4/src/ch4i_comm.c          \
                    src/mpid/ch4/src/ch4r_init.c          \
                    src/mpid/ch4/src/ch4r_proc.c          \
                    src/mpid/ch4/src/ch4r_recvq.c         \
                    src/mpid/ch4/src/ch4r_callbacks.c     \
                    src/mpid/ch4/src/ch4r_rma_origin_callbacks.c     \
                    src/mpid/ch4/src/ch4r_rma_target_callbacks.c     \
                    src/mpid/ch4/src/ch4r_win.c           \
                    src/mpid/ch4/src/mpid_ch4_net_array.c

if BUILD_CH4_COLL_TUNING
mpi_core_sources += src/mpid/ch4/src/ch4_coll_globals.c
endif

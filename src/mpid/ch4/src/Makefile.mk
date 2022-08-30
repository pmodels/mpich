##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/src

noinst_HEADERS += src/mpid/ch4/src/ch4_comm.h     \
                  src/mpid/ch4/src/ch4_progress.h \
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
                  src/mpid/ch4/src/ch4_part.h     \
                  src/mpid/ch4/src/ch4_proc.h     \
                  src/mpid/ch4/src/ch4i_comm.h    \
		  src/mpid/ch4/src/mpidig.h \
                  src/mpid/ch4/src/mpidig_util.h \
                  src/mpid/ch4/src/mpidig_request.h \
                  src/mpid/ch4/src/mpidig_recvq.h \
                  src/mpid/ch4/src/mpidig_pt2pt_callbacks.h \
                  src/mpid/ch4/src/mpidig_rma_callbacks.h \
		  src/mpid/ch4/src/mpidig_send.h \
		  src/mpid/ch4/src/mpidig_recv.h \
                  src/mpid/ch4/src/mpidig_probe.h \
                  src/mpid/ch4/src/mpidig_rma.h \
                  src/mpid/ch4/src/mpidig_win.h \
		  src/mpid/ch4/src/mpidig_send_utils.h \
		  src/mpid/ch4/src/mpidig_recv_utils.h \
		  src/mpid/ch4/src/mpidig_req_cache.h \
		  src/mpid/ch4/src/mpidig_part.h \
		  src/mpid/ch4/src/mpidig_part_callbacks.h \
		  src/mpid/ch4/src/mpidig_part_utils.h

mpi_core_sources += src/mpid/ch4/src/ch4_globals.c        \
                    src/mpid/ch4/src/ch4_impl.c           \
                    src/mpid/ch4/src/ch4_init.c           \
                    src/mpid/ch4/src/init_comm.c          \
                    src/mpid/ch4/src/ch4_comm.c           \
                    src/mpid/ch4/src/ch4_spawn.c          \
                    src/mpid/ch4/src/ch4_win.c            \
                    src/mpid/ch4/src/ch4_part.c           \
                    src/mpid/ch4/src/ch4_self.c           \
                    src/mpid/ch4/src/ch4i_comm.c          \
                    src/mpid/ch4/src/ch4_proc.c           \
                    src/mpid/ch4/src/ch4_stream_enqueue.c \
		    src/mpid/ch4/src/mpidig_init.c \
                    src/mpid/ch4/src/mpidig_recvq.c \
                    src/mpid/ch4/src/mpidig_pt2pt_callbacks.c \
                    src/mpid/ch4/src/mpidig_rma_callbacks.c \
                    src/mpid/ch4/src/mpidig_win.c \
		    src/mpid/ch4/src/mpidig_part.c \
		    src/mpid/ch4/src/mpidig_part_callbacks.c \
		    src/mpid/ch4/src/mpidig_comm_abort.c \
                    src/mpid/ch4/src/mpid_ch4_net_array.c

if BUILD_CH4_COLL_TUNING
mpi_core_sources += src/mpid/ch4/src/ch4_coll_globals.c
endif

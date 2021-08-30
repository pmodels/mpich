##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/ipc/src

noinst_HEADERS += src/mpid/ch4/shm/ipc/src/shm_inline.h    \
                  src/mpid/ch4/shm/ipc/src/ipc_noinline.h  \
                  src/mpid/ch4/shm/ipc/src/ipc_send.h      \
                  src/mpid/ch4/shm/ipc/src/ipc_p2p.h       \
                  src/mpid/ch4/shm/ipc/src/ipc_types.h     \
                  src/mpid/ch4/shm/ipc/src/ipc_pre.h

mpi_core_sources += src/mpid/ch4/shm/ipc/src/globals.c     \
                    src/mpid/ch4/shm/ipc/src/ipc_init.c    \
                    src/mpid/ch4/shm/ipc/src/ipc_control.c \
                    src/mpid/ch4/shm/ipc/src/ipc_win.c


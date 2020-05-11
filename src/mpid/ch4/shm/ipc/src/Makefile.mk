##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

noinst_HEADERS += src/mpid/ch4/shm/ipc/src/shm_inline.h    \
                  src/mpid/ch4/shm/ipc/src/ipc_noinline.h  \
                  src/mpid/ch4/shm/ipc/src/ipc_impl.h      \
                  src/mpid/ch4/shm/ipc/src/ipc_send.h      \
                  src/mpid/ch4/shm/ipc/src/ipc_recv.h      \
                  src/mpid/ch4/shm/ipc/src/ipc_control.h   \
                  src/mpid/ch4/shm/ipc/src/ipc_pre.h

mpi_core_sources += src/mpid/ch4/shm/ipc/src/globals.c     \
                    src/mpid/ch4/shm/ipc/src/ipc_init.c    \
                    src/mpid/ch4/shm/ipc/src/ipc_control.c \
                    src/mpid/ch4/shm/ipc/src/ipc_win.c


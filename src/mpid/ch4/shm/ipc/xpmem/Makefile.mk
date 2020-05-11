##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_IPC_XPMEM

noinst_HEADERS += src/mpid/ch4/shm/ipc/xpmem/xpmem_noinline.h  \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_impl.h      \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_seg.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_send.h      \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_recv.h      \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_control.h   \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_mem.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_pre.h

mpi_core_sources += src/mpid/ch4/shm/ipc/xpmem/globals.c       \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_init.c    \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_control.c \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_win.c
endif

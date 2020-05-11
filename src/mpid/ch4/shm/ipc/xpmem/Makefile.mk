##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_IPC_XPMEM

noinst_HEADERS += src/mpid/ch4/shm/ipc/xpmem/xpmem_noinline.h  \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_seg.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_types.h     \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_pre.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_post.h

mpi_core_sources += src/mpid/ch4/shm/ipc/xpmem/globals.c       \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_init.c    \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_seg.c

endif

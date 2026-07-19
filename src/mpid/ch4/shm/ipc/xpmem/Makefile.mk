##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

noinst_HEADERS += src/mpid/ch4/shm/ipc/xpmem/xpmem_pre.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_post.h

if BUILD_SHM_IPC_XPMEM
noinst_HEADERS += src/mpid/ch4/shm/ipc/xpmem/xpmem_seg.h       \
                  src/mpid/ch4/shm/ipc/xpmem/xpmem_types.h

mpi_core_sources += src/mpid/ch4/shm/ipc/xpmem/globals.c       \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_init.c    \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_mem.c     \
                    src/mpid/ch4/shm/ipc/xpmem/xpmem_seg.c
endif

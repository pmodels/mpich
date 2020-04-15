##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_XPMEM

noinst_HEADERS += src/mpid/ch4/shm/xpmem/shm_inline.h      \
                  src/mpid/ch4/shm/xpmem/xpmem_noinline.h  \
                  src/mpid/ch4/shm/xpmem/xpmem_impl.h      \
                  src/mpid/ch4/shm/xpmem/xpmem_seg.h       \
                  src/mpid/ch4/shm/xpmem/xpmem_send.h      \
                  src/mpid/ch4/shm/xpmem/xpmem_recv.h      \
                  src/mpid/ch4/shm/xpmem/xpmem_control.h   \
                  src/mpid/ch4/shm/xpmem/xpmem_pre.h

mpi_core_sources += src/mpid/ch4/shm/xpmem/globals.c       \
                    src/mpid/ch4/shm/xpmem/xpmem_init.c    \
                    src/mpid/ch4/shm/xpmem/xpmem_control.c \
                    src/mpid/ch4/shm/xpmem/xpmem_win.c
endif

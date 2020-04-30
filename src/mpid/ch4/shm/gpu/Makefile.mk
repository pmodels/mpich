##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_GPU_IPC

noinst_HEADERS += src/mpid/ch4/shm/gpu/shm_inline.h    \
                  src/mpid/ch4/shm/gpu/gpu_noinline.h  \
                  src/mpid/ch4/shm/gpu/gpu_impl.h      \
                  src/mpid/ch4/shm/gpu/gpu_send.h      \
                  src/mpid/ch4/shm/gpu/gpu_recv.h      \
                  src/mpid/ch4/shm/gpu/gpu_control.h   \
                  src/mpid/ch4/shm/gpu/gpu_pre.h

mpi_core_sources += src/mpid/ch4/shm/gpu/globals.c     \
                    src/mpid/ch4/shm/gpu/gpu_init.c    \
                    src/mpid/ch4/shm/gpu/gpu_control.c
endif

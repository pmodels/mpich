##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

noinst_HEADERS += src/mpid/ch4/shm/ipc/gpu/gpu_noinline.h  \
                  src/mpid/ch4/shm/ipc/gpu/gpu_pre.h       \
                  src/mpid/ch4/shm/ipc/gpu/gpu_post.h

if BUILD_SHM_IPC_GPU
mpi_core_sources += src/mpid/ch4/shm/ipc/gpu/gpu_init.c
endif

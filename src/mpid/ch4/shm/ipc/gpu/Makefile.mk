##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

noinst_HEADERS += src/mpid/ch4/shm/ipc/gpu/gpu_pre.h       \
                  src/mpid/ch4/shm/ipc/gpu/gpu_post.h

mpi_core_sources += src/mpid/ch4/shm/ipc/gpu/gpu_post.c
if BUILD_SHM_IPC_GPU
mpi_core_sources += src/mpid/ch4/shm/ipc/gpu/globals.c     \
                    src/mpid/ch4/shm/ipc/gpu/gpu_init.c

noinst_HEADERS += src/mpid/ch4/shm/ipc/gpu/gpu_types.h
else
mpi_core_sources += src/mpid/ch4/shm/ipc/gpu/gpu_stub.c
endif

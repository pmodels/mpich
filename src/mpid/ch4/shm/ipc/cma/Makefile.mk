##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_IPC_CMA
mpi_core_sources += \
    src/mpid/ch4/shm/ipc/cma/cma_init.c \
    src/mpid/ch4/shm/ipc/cma/cma_post.c
endif

##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_SHM_IPC_CMA
mpi_core_sources += \
    src/mpid/ch4/shm/ipc/cma/cma_init.c \
    src/mpid/ch4/shm/ipc/cma/cma_post.c
endif

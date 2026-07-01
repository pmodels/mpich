##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

lib@MPLLIBNAME@_la_SOURCES +=        \
    src/shm/mpl_shm.c                \
    src/shm/mpl_shm_sysv.c            \
    src/shm/mpl_shm_mmap.c            \
    src/shm/mpl_shm_win.c             \
    src/shm/mpl_shm_posix.c

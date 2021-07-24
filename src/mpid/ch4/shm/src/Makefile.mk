##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/src

noinst_HEADERS += src/mpid/ch4/shm/src/shm_impl.h    \
        src/mpid/ch4/shm/src/shm_am_fallback.h       \
        src/mpid/ch4/shm/src/shm_am_fallback_coll.h  \
        src/mpid/ch4/shm/src/shm_am_fallback_probe.h \
        src/mpid/ch4/shm/src/shm_am_fallback_recv.h  \
        src/mpid/ch4/shm/src/shm_am_fallback_rma.h   \
        src/mpid/ch4/shm/src/shm_am_fallback_send.h  \
        src/mpid/ch4/shm/src/shm_am_fallback_part.h  \
        src/mpid/ch4/shm/src/shm_am.h      \
        src/mpid/ch4/shm/src/shm_coll.h    \
        src/mpid/ch4/shm/src/shm_hooks.h   \
        src/mpid/ch4/shm/src/shm_progress.h \
        src/mpid/ch4/shm/src/shm_p2p.h     \
        src/mpid/ch4/shm/src/shm_part.h    \
        src/mpid/ch4/shm/src/shm_noinline.h\
        src/mpid/ch4/shm/src/shm_rma.h

mpi_core_sources   += src/mpid/ch4/shm/src/shm_init.c \
                      src/mpid/ch4/shm/src/shm_hooks.c \
                      src/mpid/ch4/shm/src/shm_mem.c \
                      src/mpid/ch4/shm/src/shm_rma.c \
                      src/mpid/ch4/shm/src/shm_part.c

noinst_HEADERS += src/mpid/ch4/shm/src/topotree_util.h \
                  src/mpid/ch4/shm/src/topotree_types.h\
                  src/mpid/ch4/shm/src/topotree.h

mpi_core_sources   += src/mpid/ch4/shm/src/topotree.c \
                      src/mpid/ch4/shm/src/topotree_util.c

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/util/ccl/Makefile.mk

mpi_core_sources +=   \
    src/util/mpir_assert.c     \
    src/util/mpir_cvars.c      \
    src/util/mpir_pmi.c        \
    src/util/mpir_nodemap.c    \
    src/util/mpir_handlemem.c  \
    src/util/mpir_strerror.c   \
    src/util/mpir_netloc.c     \
    src/util/mpir_hwtopo.c     \
    src/util/mpir_nettopo.c    \
    src/util/mpir_async_things.c \
    src/util/mpir_progress_hook.c

## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_sources +=                \
        src/mpi_t/cat_changed.c         \
        src/mpi_t/cat_get_categories.c  \
        src/mpi_t/cat_get_cvars.c       \
        src/mpi_t/cat_get_info.c        \
        src/mpi_t/cat_get_num.c         \
        src/mpi_t/cat_get_pvars.c       \
        src/mpi_t/cvar_get_info.c       \
        src/mpi_t/cvar_get_num.c        \
        src/mpi_t/cvar_handle_alloc.c   \
        src/mpi_t/cvar_handle_free.c    \
        src/mpi_t/cvar_read.c           \
        src/mpi_t/cvar_write.c          \
        src/mpi_t/enum_get_info.c       \
        src/mpi_t/enum_get_item.c       \
        src/mpi_t/mpit_finalize.c       \
        src/mpi_t/mpit_initthread.c     \
        src/mpi_t/pvar_get_info.c       \
        src/mpi_t/pvar_get_num.c        \
        src/mpi_t/pvar_handle_alloc.c   \
        src/mpi_t/pvar_handle_free.c    \
        src/mpi_t/pvar_read.c           \
        src/mpi_t/pvar_readreset.c      \
        src/mpi_t/pvar_reset.c          \
        src/mpi_t/pvar_session_create.c \
        src/mpi_t/pvar_session_free.c   \
        src/mpi_t/pvar_start.c          \
        src/mpi_t/pvar_stop.c           \
        src/mpi_t/pvar_write.c          \
        src/mpi_t/cat_get_index.c       \
        src/mpi_t/cvar_get_index.c      \
        src/mpi_t/pvar_get_index.c


mpi_core_sources += src/mpi_t/mpit.c


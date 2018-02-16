## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/shm/glue

noinst_HEADERS += src/mpid/ch4/shm/glue/shm_impl.h  \
        src/mpid/ch4/shm/glue/shm_am.h      \
        src/mpid/ch4/shm/glue/shm_coll.h    \
        src/mpid/ch4/shm/glue/shm_dpm.h     \
        src/mpid/ch4/shm/glue/shm_hooks.h   \
        src/mpid/ch4/shm/glue/shm_init.h    \
        src/mpid/ch4/shm/glue/shm_mem.h     \
        src/mpid/ch4/shm/glue/shm_misc.h    \
        src/mpid/ch4/shm/glue/shm_p2p.h     \
        src/mpid/ch4/shm/glue/shm_startall.h\
        src/mpid/ch4/shm/glue/shm_rma.h

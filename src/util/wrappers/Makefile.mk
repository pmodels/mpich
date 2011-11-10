## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## FIXME check that this is the right way to do VPATH
AM_CPPFLAGS += -I$(top_srcdir)/src/util/wrappers -I$(top_builddir)/src/util/wrappers

noinst_HEADERS +=                             \
    src/util/wrappers/mpiu_os_wrappers.h      \
    src/util/wrappers/mpiu_os_wrappers_pre.h  \
    src/util/wrappers/mpiu_process_wrappers.h \
    src/util/wrappers/mpiu_shm_wrappers.h     \
    src/util/wrappers/mpiu_sock_wrappers.h    \
    src/util/wrappers/mpiu_util_wrappers.h


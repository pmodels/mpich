## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
if BUILD_CH4_NETMOD_UCX

noinst_HEADERS     +=
mpi_core_sources   += src/mpid/ch4/netmod/ucx/func_table.c\
                      src/mpid/ch4/netmod/ucx/globals.c

errnames_txt_files += src/mpid/ch4/netmod/ucx/errnames.txt

external_subdirs   += @ucxdir@
pmpi_convenience_libs += @ucxlib@

endif

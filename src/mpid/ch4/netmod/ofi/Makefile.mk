## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##
if BUILD_CH4_NETMOD_OFI

noinst_HEADERS     +=
mpi_core_sources   += src/mpid/ch4/netmod/ofi/func_table.c \
                      src/mpid/ch4/netmod/ofi/ofi_init.c \
                      src/mpid/ch4/netmod/ofi/ofi_comm.c \
                      src/mpid/ch4/netmod/ofi/ofi_datatype.c \
                      src/mpid/ch4/netmod/ofi/ofi_op.c \
                      src/mpid/ch4/netmod/ofi/ofi_spawn.c \
                      src/mpid/ch4/netmod/ofi/ofi_win.c \
                      src/mpid/ch4/netmod/ofi/ofi_events.c \
                      src/mpid/ch4/netmod/ofi/ofi_progress.c \
                      src/mpid/ch4/netmod/ofi/globals.c \
                      src/mpid/ch4/netmod/ofi/util.c
errnames_txt_files += src/mpid/ch4/netmod/ofi/errnames.txt

endif

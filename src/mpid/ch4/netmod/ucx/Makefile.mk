##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4_NETMOD_UCX

noinst_HEADERS     +=
mpi_core_sources   += src/mpid/ch4/netmod/ucx/func_table.c\
                      src/mpid/ch4/netmod/ucx/ucx_init.c \
                      src/mpid/ch4/netmod/ucx/ucx_comm.c \
                      src/mpid/ch4/netmod/ucx/ucx_datatype.c \
                      src/mpid/ch4/netmod/ucx/ucx_op.c \
                      src/mpid/ch4/netmod/ucx/ucx_spawn.c \
                      src/mpid/ch4/netmod/ucx/ucx_win.c \
                      src/mpid/ch4/netmod/ucx/ucx_part.c \
                      src/mpid/ch4/netmod/ucx/ucx_progress.c \
                      src/mpid/ch4/netmod/ucx/globals.c

errnames_txt_files += src/mpid/ch4/netmod/ucx/errnames.txt

external_subdirs   += @ucxdir@
pmpi_convenience_libs += @ucxlib@

endif

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4_NETMOD_STUBNM

mpi_core_sources   += src/mpid/ch4/netmod/stubnm/func_table.c\
                      src/mpid/ch4/netmod/stubnm/stubnm_init.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_comm.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_datatype.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_op.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_spawn.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_win.c \
                      src/mpid/ch4/netmod/stubnm/stubnm_progress.c
# errnames_txt_files += src/mpid/ch4/netmod/stub/errnames.txt

endif

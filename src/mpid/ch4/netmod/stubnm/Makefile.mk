##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4_NETMOD_STUBNM

mpi_core_sources   += src/mpid/ch4/netmod/stubnm/func_table.c\
                      src/mpid/ch4/netmod/stubnm/stubnm_noinline.c

# errnames_txt_files += src/mpid/ch4/netmod/stub/errnames.txt

endif

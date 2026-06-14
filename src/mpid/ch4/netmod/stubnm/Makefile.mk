##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_CH4_NETMOD_STUBNM

mpi_core_sources   += src/mpid/ch4/netmod/stubnm/func_table.c\
                      src/mpid/ch4/netmod/stubnm/stubnm_noinline.c

# errnames_txt_files += src/mpid/ch4/netmod/stub/errnames.txt

endif

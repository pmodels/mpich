##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_SHM_STUBSHM

mpi_core_sources += src/mpid/ch4/shm/stubshm/stubshm_noinline.c

# errnames_txt_files += src/mpid/ch4/shm/stub/errnames.txt

endif

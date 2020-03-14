##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_SHM_STUBSHM

mpi_core_sources += src/mpid/ch4/shm/stubshm/globals.c \
                    src/mpid/ch4/shm/stubshm/stubshm_comm.c \
                    src/mpid/ch4/shm/stubshm/stubshm_init.c \
                    src/mpid/ch4/shm/stubshm/stubshm_spawn.c \
                    src/mpid/ch4/shm/stubshm/stubshm_progress.c \
                    src/mpid/ch4/shm/stubshm/stubshm_win.c
# errnames_txt_files += src/mpid/ch4/shm/stub/errnames.txt

endif

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4_NETMOD_SOCK

noinst_HEADERS     +=
mpi_core_sources   += src/mpid/ch4/netmod/sock/func_table.c \
                      src/mpid/ch4/netmod/sock/sock_init.c \
                      src/mpid/ch4/netmod/sock/sock_comm.c \
                      src/mpid/ch4/netmod/sock/sock_datatype.c \
                      src/mpid/ch4/netmod/sock/sock_op.c \
                      src/mpid/ch4/netmod/sock/sock_spawn.c \
                      src/mpid/ch4/netmod/sock/sock_win.c \
                      src/mpid/ch4/netmod/sock/sock_part.c \
                      src/mpid/ch4/netmod/sock/globals.c

endif

## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

mpi_core_sources +=				\
    src/mpid/ch3/channels/nemesis/src/ch3_finalize.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_init.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_isend.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_isendv.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_istartmsg.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_istartmsgv.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_progress.c		\
    src/mpid/ch3/channels/nemesis/src/ch3_rma_shm.c             \
    src/mpid/ch3/channels/nemesis/src/ch3_win_fns.c             \
    src/mpid/ch3/channels/nemesis/src/ch3i_comm.c		\
    src/mpid/ch3/channels/nemesis/src/ch3i_eagernoncontig.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_alloc.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_init.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_barrier.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_mpich.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_ckpt.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_finalize.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_network_poll.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_network.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_debug.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt.c		\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt_shm.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_net_array.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt_dma.c	\
    src/mpid/ch3/channels/nemesis/src/mpid_nem_lmt_vmsplice.c


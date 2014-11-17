## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
if BUILD_NEMESIS_NETMOD_SFI

mpi_core_sources +=                                 		\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_init.c 	\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_cm.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_tagged.c	\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_msg.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_data.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/sfi/sfi_progress.c

errnames_txt_files += src/mpid/ch3/channels/nemesis/netmod/sfi/errnames.txt

endif

## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
if BUILD_NEMESIS_NETMOD_OFI

mpi_core_sources +=                                 		\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_init.c 	\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_cm.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_tagged.c	\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_msg.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_data.c	 	\
    src/mpid/ch3/channels/nemesis/netmod/ofi/ofi_progress.c

errnames_txt_files += src/mpid/ch3/channels/nemesis/netmod/ofi/errnames.txt

endif

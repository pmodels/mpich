## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## TODO handle the other netmods
#include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/elan/Makefile.mk
#include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/gm/Makefile.mk
#include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/mx/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/tcp/Makefile.mk
include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/none/Makefile.mk
#include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/psm/Makefile.mk
#include $(top_srcdir)/src/mpid/ch3/channels/nemesis/nemesis/netmod/newmad/Makefile.mk


## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2016 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.
##

if BUILD_CH4

include $(top_srcdir)/src/mpid/ch4/include/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/generic/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/netmod/Makefile.mk
if BUILD_CH4_SHM
include $(top_srcdir)/src/mpid/ch4/shm/Makefile.mk
endif BUILD_CH4_SHM
if BUILD_CH4_GPU
include $(top_srcdir)/src/mpid/ch4/gpu/Makefile.mk
endif BUILD_CH4_GPU
endif BUILD_CH4

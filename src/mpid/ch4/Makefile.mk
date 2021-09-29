##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_CH4

include $(top_srcdir)/src/mpid/ch4/include/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/netmod/Makefile.mk
if BUILD_CH4_SHM
include $(top_srcdir)/src/mpid/ch4/shm/Makefile.mk
endif BUILD_CH4_SHM

endif BUILD_CH4

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/include

noinst_HEADERS += \
	src/backend/hip/include/yaksuri_hip_pre.h \
	src/backend/hip/include/yaksuri_hip_post.h \
	src/backend/hip/include/yaksuri_hipi_base.h \
	src/backend/hip/include/yaksuri_hipi.h

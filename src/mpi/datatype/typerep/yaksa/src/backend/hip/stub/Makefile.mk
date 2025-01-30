##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/stub

noinst_HEADERS += \
	src/backend/hip/stub/yaksuri_hip_pre.h \
	src/backend/hip/stub/yaksuri_hip_post.h

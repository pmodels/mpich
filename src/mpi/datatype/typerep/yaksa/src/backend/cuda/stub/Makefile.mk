##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/stub

noinst_HEADERS += \
	src/backend/cuda/stub/yaksuri_cuda_pre.h \
	src/backend/cuda/stub/yaksuri_cuda_post.h

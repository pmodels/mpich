##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/include

noinst_HEADERS += \
	src/backend/ze/include/yaksuri_ze_pre.h \
	src/backend/ze/include/yaksuri_ze_post.h \
	src/backend/ze/include/yaksuri_zei.h \
	src/backend/ze/include/yaksuri_zei_md.h

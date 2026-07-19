##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/src

libyaksa_la_SOURCES += \
	src/backend/src/yaksuri_progress.c \
	src/backend/src/yaksur_hooks.c \
	src/backend/src/yaksur_pup.c \
	src/backend/src/yaksur_request.c

noinst_HEADERS += \
	src/backend/src/yaksuri.h \
	src/backend/src/yaksur_pre.h \
	src/backend/src/yaksur_post.h

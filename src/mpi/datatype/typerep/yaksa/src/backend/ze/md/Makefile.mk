##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/md

libyaksa_la_SOURCES += \
	src/backend/ze/md/yaksuri_zei_md.c

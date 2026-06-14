##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/pup

libyaksa_la_SOURCES += \
	src/backend/cuda/pup/yaksuri_cudai_event.c \
	src/backend/cuda/pup/yaksuri_cudai_get_ptr_attr.c

include src/backend/cuda/pup/Makefile.pup.mk
include src/backend/cuda/pup/Makefile.populate_pupfns.mk

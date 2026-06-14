##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/types

libyaksa_la_SOURCES += \
	src/frontend/types/yaksa_vector.c \
	src/frontend/types/yaksa_contig.c \
	src/frontend/types/yaksa_dup.c \
	src/frontend/types/yaksa_resized.c \
	src/frontend/types/yaksa_blkindx.c \
	src/frontend/types/yaksa_indexed.c \
	src/frontend/types/yaksa_subarray.c \
	src/frontend/types/yaksa_struct.c \
	src/frontend/types/yaksa_free.c \
	src/frontend/types/yaksi_type.c

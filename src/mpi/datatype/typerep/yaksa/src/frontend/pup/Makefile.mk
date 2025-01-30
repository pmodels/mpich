##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/pup

libyaksa_la_SOURCES += \
	src/frontend/pup/yaksa_ipack.c \
	src/frontend/pup/yaksa_pack.c \
	src/frontend/pup/yaksa_iunpack.c \
	src/frontend/pup/yaksa_unpack.c \
	src/frontend/pup/yaksa_pack_stream.c \
	src/frontend/pup/yaksa_unpack_stream.c \
	src/frontend/pup/yaksa_request.c \
	src/frontend/pup/yaksi_ipack.c \
	src/frontend/pup/yaksi_ipack_element.c \
	src/frontend/pup/yaksi_ipack_backend.c \
	src/frontend/pup/yaksi_iunpack.c \
	src/frontend/pup/yaksi_iunpack_element.c \
	src/frontend/pup/yaksi_iunpack_backend.c \
	src/frontend/pup/yaksi_request.c

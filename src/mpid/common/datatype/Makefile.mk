## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

if BUILD_MPID_COMMON_DATATYPE

include $(top_srcdir)/src/mpid/common/datatype/dataloop/Makefile.mk

mpi_core_sources +=                          \
    src/mpid/common/datatype/mpid_contents_support.c       \
    src/mpid/common/datatype/mpid_datatype_contents.c      \
    src/mpid/common/datatype/mpid_datatype_free.c          \
    src/mpid/common/datatype/mpid_ext32_datatype.c         \
    src/mpid/common/datatype/mpid_ext32_segment.c          \
    src/mpid/common/datatype/mpid_segment.c                \
    src/mpid/common/datatype/mpid_type_blockindexed.c      \
    src/mpid/common/datatype/mpid_type_commit.c            \
    src/mpid/common/datatype/mpid_type_contiguous.c        \
    src/mpid/common/datatype/mpid_type_create_pairtype.c   \
    src/mpid/common/datatype/mpid_type_create_resized.c    \
    src/mpid/common/datatype/mpid_type_debug.c             \
    src/mpid/common/datatype/mpid_type_dup.c               \
    src/mpid/common/datatype/mpid_type_get_contents.c      \
    src/mpid/common/datatype/mpid_type_get_envelope.c      \
    src/mpid/common/datatype/mpid_type_indexed.c           \
    src/mpid/common/datatype/mpid_type_struct.c            \
    src/mpid/common/datatype/mpid_type_vector.c            \
    src/mpid/common/datatype/mpid_type_zerolen.c           \
    src/mpid/common/datatype/mpir_type_flatten.c

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/datatype

noinst_HEADERS +=                                 \
    src/mpid/common/datatype/mpid_dataloop.h      \
    src/mpid/common/datatype/mpid_datatype.h      \
    src/mpid/common/datatype/mpid_ext32_segment.h \
    src/mpid/common/datatype/segment_states.h

endif BUILD_MPID_COMMON_DATATYPE


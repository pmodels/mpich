##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_MPID_COMMON_GENQ

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/genq

noinst_HEADERS += \
		  src/mpid/common/genq/mpidu_genq.h \
		  src/mpid/common/genq/mpidu_genq_shmem_pool.h \
		  src/mpid/common/genq/mpidu_genq_shmem_queue.h \
		  src/mpid/common/genq/mpidu_genqi_shmem_types.h \
		  src/mpid/common/genq/mpidu_genq_common.h \
		  src/mpid/common/genq/mpidu_genq_private_pool.h

mpi_core_sources += \
		    src/mpid/common/genq/mpidu_genq_shmem_pool.c \
		    src/mpid/common/genq/mpidu_genq_shmem_queue.c \
		    src/mpid/common/genq/mpidu_genq_private_pool.c

endif BUILD_MPID_COMMON_GENQ

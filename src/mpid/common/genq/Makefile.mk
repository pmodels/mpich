##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

# there are no AC_OUTPUT_FILES headers, so builddir is unnecessary
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/genq

if BUILD_MPID_COMMON_GENQ_PRIVATE
mpi_core_sources += \
		    src/mpid/common/genq/mpidu_genq_private_pool.c

endif BUILD_MPID_COMMON_GENQ_PRIVATE

if BUILD_MPID_COMMON_GENQ_SHM
mpi_core_sources += \
		    src/mpid/common/genq/mpidu_genq_shmem_pool.c \
		    src/mpid/common/genq/mpidu_genq_shmem_queue.c

endif BUILD_MPID_COMMON_GENQ_SHM

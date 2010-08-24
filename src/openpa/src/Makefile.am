# -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*-
#  
#  (C) 2008 by Argonne National Laboratory.
#      See COPYRIGHT in top-level directory.

lib_LTLIBRARIES = libopa.la
libopa_la_SOURCES = opa_primitives.c opa_queue.c
libopa_la_LDFLAGS = -version-info ${libopa_so_version}

include_HEADERS = opa_config.h opa_primitives.h opa_queue.h opa_util.h 

# "nobase_" causes the primitives directory to be preserved when installed
nobase_include_HEADERS = primitives/opa_by_lock.h                   \
                         primitives/opa_emulated.h                  \
                         primitives/opa_gcc_ia64.h                  \
                         primitives/opa_gcc_intel_32_64.h           \
                         primitives/opa_gcc_intel_32_64_barrier.h   \
                         primitives/opa_gcc_intel_32_64_ops.h       \
                         primitives/opa_gcc_intel_32_64_p3.h        \
                         primitives/opa_gcc_intel_32_64_p3barrier.h \
                         primitives/opa_gcc_intrinsics.h            \
                         primitives/opa_gcc_ppc.h                   \
                         primitives/opa_gcc_sicortex.h              \
                         primitives/opa_nt_intrinsics.h             \
                         primitives/opa_sun_atomic_ops.h            \
                         primitives/opa_unsafe.h

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if MPL_HAVE_CUDA
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_cuda.c
else
lib@MPLLIBNAME@_la_SOURCES += src/gpu/mpl_gpu_fallback.c
endif

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if MPL_BUILD_FOR_X86

lib@MPLLIBNAME@_la_SOURCES += src/arch/mpl_arch_x86.c

src/arch/mpl_arch_x86.lo: CFLAGS = $(NO_OPT_CFLAGS)

else

lib@MPLLIBNAME@_la_SOURCES += src/arch/mpl_arch_general.c

endif

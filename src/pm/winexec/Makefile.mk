##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Single-node process manager for native Windows (see mpiexec.c)
if BUILD_PM_WINEXEC
bin_PROGRAMS += src/pm/winexec/mpiexec
src_pm_winexec_mpiexec_SOURCES = src/pm/winexec/mpiexec.c
src_pm_winexec_mpiexec_LDADD = $(mpl_lib)
EXTRA_src_pm_winexec_mpiexec_DEPENDENCIES = $(mpl_lib)
src_pm_winexec_mpiexec_CPPFLAGS = $(AM_CPPFLAGS)
endif BUILD_PM_WINEXEC

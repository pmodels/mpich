## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2012 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## NOTE: this is subtly different from the simplemake strategy.  The simplemake
## approach always built "mpiexec" from "mpiexec.c" and then either the
## "install" or the "install-alt" target was invoked by the parent directory's
## make process, which would install "mpiexec" or "mpiexec.remshell", respectively.
## Instead, we build the binary by the final installed name, which we determine
## before install-time.

if BUILD_PM_REMSHELL
if PRIMARY_PM_REMSHELL
bin_PROGRAMS += src/pm/remshell/mpiexec
src_pm_remshell_mpiexec_SOURCES = src/pm/remshell/mpiexec.c 
src_pm_remshell_mpiexec_LDADD = src/pm/util/libmpiexec.a
# we may not want to add AM_CPPFLAGS for this program
src_pm_remshell_mpiexec_CPPFLAGS = $(common_pm_includes) $(AM_CPPFLAGS)
else !PRIMARY_PM_REMSHELL
bin_PROGRAMS += src/pm/remshell/mpiexec.remshell
src_pm_remshell_mpiexec_remshell_SOURCES = src/pm/remshell/mpiexec.c 
src_pm_remshell_mpiexec_remshell_LDADD = src/pm/util/libmpiexec.a
# we may not want to add AM_CPPFLAGS for this program
src_pm_remshell_mpiexec_remshell_CPPFLAGS = $(common_pm_includes) $(AM_CPPFLAGS)
endif !PRIMARY_PM_REMSHELL
endif BUILD_PM_REMSHELL


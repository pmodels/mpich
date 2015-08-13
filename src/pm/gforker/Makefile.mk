## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

## NOTE: this is subtly different from the simplemake strategy.  The simplemake
## approach always built "mpiexec" from "mpiexec.c" and then either the
## "install" or the "install-alt" target was invoked by the parent directory's
## make process, which would install "mpiexec" or "mpiexec.gforker", respectively.
## Instead, we build the binary by the final installed name, which we determine
## before install-time.

if BUILD_PM_GFORKER
if PRIMARY_PM_GFORKER
bin_PROGRAMS += src/pm/gforker/mpiexec
src_pm_gforker_mpiexec_SOURCES = src/pm/gforker/mpiexec.c 
src_pm_gforker_mpiexec_LDADD = src/pm/util/libmpiexec.a $(mpllib)
# we may not want to add AM_CPPFLAGS for this program
src_pm_gforker_mpiexec_CPPFLAGS = $(common_pm_includes) $(AM_CPPFLAGS)
else !PRIMARY_PM_GFORKER
bin_PROGRAMS += src/pm/gforker/mpiexec.gforker
src_pm_gforker_mpiexec_gforker_SOURCES = src/pm/gforker/mpiexec.c 
src_pm_gforker_mpiexec_gforker_LDADD = src/pm/util/libmpiexec.a
# we may not want to add AM_CPPFLAGS for this program
src_pm_gforker_mpiexec_gforker_CPPFLAGS = $(common_pm_includes) $(AM_CPPFLAGS)
endif !PRIMARY_PM_GFORKER
endif BUILD_PM_GFORKER

## TODO convert these simplemake doc commands to the new scheme
##doc_sources = mpiexec.txt
##DOCDESTDIRS = html:www/www1,man:man/man1,latex:doc/refman
##docargs_ADD       = ${master_top_srcdir}/doc/mansrc/cmdnotes
##doc_HTML_SOURCES  = ${doc_sources}
##doc_MAN_SOURCES   = ${doc_sources}
##doc_LATEX_SOURCES = ${doc_sources}


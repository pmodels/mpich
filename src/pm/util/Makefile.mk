## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# common include paths that are used by gforker, remshell, and any other process
# managers that use utility code from this directory
common_pm_includes = \
    -I${top_builddir}/src/include -I${top_srcdir}/src/include \
    -I${top_builddir}/src/pmi/simple -I${top_srcdir}/src/pmi/simple \
    -I${top_builddir}/src/pm/util -I${top_srcdir}/src/pm/util

if BUILD_PM_UTIL

## FIXME do we need this still?
##OTHER_DIRS = test

noinst_LTLIBRARIES += src/pm/util/libmpiexec.la

# Ensure that dgbiface is compiled with the -g option, as the symbols must
# be present for the debugger to see them
src_pm_util_libmpiexec_la_CFLAGS = -g $(AM_CFLAGS)

# we may want to omit the regular AM_CPPFLAGS when building objects in this
# utility library
src_pm_util_libmpiexec_la_CPPFLAGS = $(common_pm_includes) $(AM_CPPFLAGS)

# MPL
src_pm_util_libmpiexec_la_LIBADD = $(mpllib)
src_pm_util_libmpiexec_la_LDFLAGS = $(mpllibdir)
EXTRA_src_pm_util_libmpiexec_la_DEPENDENCIES = $(mpllib)

# We use the msg print routines (for now) - include these in the mpiexec
# library so that we don't need to copy the source files
# simple_pmiutil2 is a subset of simple_pmiutil,
# since it may no longer be used by other applications
# (they make use of routines like the trmem routines that may no longer
# be used by other applications).
#
# [goodell] FIXME the above comment is basically unintelligible...
src_pm_util_libmpiexec_la_SOURCES = \
    src/pm/util/cmnargs.c          \
    src/pm/util/process.c          \
    src/pm/util/ioloop.c           \
    src/pm/util/pmiserv.c          \
    src/pm/util/labelout.c         \
    src/pm/util/env.c              \
    src/pm/util/newsession.c       \
    src/pm/util/rm.c               \
    src/pm/util/pmiport.c          \
    src/pm/util/dbgiface.c         \
    src/pm/util/simple_pmiutil2.c

endif BUILD_PM_UTIL

##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# ensure that the buildiface script ends up in the release tarball
EXTRA_DIST += src/binding/fortran/mpif_h/buildiface

mpi_f77_sources += src/binding/fortran/mpif_h/attr_proxy.c

if BUILD_F77_BINDING

mpi_f77_sources += \
	src/binding/fortran/mpif_h/fortran_binding.c \
	src/binding/fortran/mpif_h/fdebug.c \
	src/binding/fortran/mpif_h/setbot.c \
	src/binding/fortran/mpif_h/setbotf.f

mpi_sources += src/binding/fortran/mpif_h/statusf2c.c src/binding/fortran/mpif_h/statusc2f.c

# FIXME does AM_CPPFLAGS need to be included elsewhere somehow in the
# target-specific variable?
AM_CPPFLAGS += -I${main_top_srcdir}/src/binding/fortran/mpif_h


noinst_HEADERS += \
	src/binding/fortran/mpif_h/fortran_profile.h \
	src/binding/fortran/mpif_h/mpi_fortimpl.h

# config.status copies src/binding/fortran/mpif_h/mpif.h to src/include (see the relevant
# AC_CONFIG_COMMANDS in configure.ac), so we need to delete it at distclean time
# too.  More work is needed in this Makefile.mk to keep src/include/mpif.h up to
# date w.r.t. the src/binding/fortran/mpif_h version.
DISTCLEANFILES += src/binding/fortran/mpif_h/mpif.h src/include/mpif.h
nodist_include_HEADERS += src/binding/fortran/mpif_h/mpif.h


endif BUILD_F77_BINDING


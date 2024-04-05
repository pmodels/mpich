##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

f77_cppflags = $(AM_CPPFLAGS) -I${main_top_srcdir}/src/binding/fortran/mpif_h

if BUILD_F77_BINDING

mpifort_convenience_libs += lib/libf77_mpi.la
noinst_LTLIBRARIES += lib/libf77_mpi.la

lib_libf77_mpi_la_SOURCES = \
	src/binding/fortran/mpif_h/fortran_binding.c \
	src/binding/fortran/mpif_h/attr_proxy.c \
	src/binding/fortran/mpif_h/user_proxy.c \
	src/binding/fortran/mpif_h/fdebug.c \
	src/binding/fortran/mpif_h/setbot.c \
	src/binding/fortran/mpif_h/setbotf.f

lib_libf77_mpi_la_CPPFLAGS = $(f77_cppflags)

if BUILD_PROFILING_LIB
mpifort_convenience_libs += lib/libf77_pmpi.la
noinst_LTLIBRARIES += lib/libf77_pmpi.la

lib_libf77_pmpi_la_SOURCES = src/binding/fortran/mpif_h/fortran_binding.c

# build "pmpi_xxx_" f77 public functions
lib_libf77_pmpi_la_CPPFLAGS = $(f77_cppflags) -DF77_USE_PMPI

# build "mpi_xxx_" f77 public functions
lib_libf77_mpi_la_CPPFLAGS += -DMPICH_MPI_FROM_PMPI -DUSE_ONLY_MPI_NAMES
endif BUILD_PROFILING_LIB

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


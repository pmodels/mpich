##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

f77_cppflags = $(AM_CPPFLAGS) -I${srcdir}/mpif_h

if BUILD_F77_BINDING

mpifort_convenience_libs += lib/libf77_mpi.la
noinst_LTLIBRARIES += lib/libf77_mpi.la

lib_libf77_mpi_la_SOURCES = \
	mpif_h/fortran_binding.c \
	mpif_h/user_proxy.c \
	mpif_h/fdebug.c \
	mpif_h/setbot.c \
        mpif_h/cmblk.c \
	mpif_h/setbotf.f

lib_libf77_mpi_la_CPPFLAGS = $(f77_cppflags)

if BUILD_PROFILING_LIB
mpifort_convenience_libs += lib/libf77_pmpi.la
noinst_LTLIBRARIES += lib/libf77_pmpi.la

lib_libf77_pmpi_la_SOURCES = mpif_h/fortran_binding.c

# build "pmpi_xxx_" f77 public functions
lib_libf77_pmpi_la_CPPFLAGS = $(f77_cppflags) -DF77_USE_PMPI

# build "mpi_xxx_" f77 public functions
lib_libf77_mpi_la_CPPFLAGS += -DMPICH_MPI_FROM_PMPI -DUSE_ONLY_MPI_NAMES
endif BUILD_PROFILING_LIB

noinst_HEADERS += \
	mpif_h/fortran_profile.h \
	mpif_h/mpi_fortimpl.h

DISTCLEANFILES += mpif_h/mpif.h
nodist_include_HEADERS += mpif_h/mpif.h


endif BUILD_F77_BINDING


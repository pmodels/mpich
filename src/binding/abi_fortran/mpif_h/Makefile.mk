##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

f77_cppflags = $(AM_CPPFLAGS) -Impif_h

if BUILD_F77_BINDING

include_HEADERS += mpif_h/mpif.h

mpifort_convenience_libs += libf77_mpi.la

libf77_mpi_la_SOURCES = \
	mpif_h/fortran_binding.c \
	mpif_h/attr_proxy.c \
	mpif_h/fdebug.c \
	mpif_h/setbot.c \
	mpif_h/setbotf.f

libf77_mpi_la_CPPFLAGS = $(f77_cppflags)

if BUILD_PROFILING_LIB
mpifort_convenience_libs += libf77_pmpi.la

libf77_pmpi_la_SOURCES = mpif_h/fortran_binding.c

# build "pmpi_xxx_" f77 public functions
libf77_pmpi_la_CPPFLAGS = $(f77_cppflags) -DF77_USE_PMPI

# build "mpi_xxx_" f77 public functions
libf77_mpi_la_CPPFLAGS += -DMPICH_MPI_FROM_PMPI -DUSE_ONLY_MPI_NAMES
endif BUILD_PROFILING_LIB

endif BUILD_F77_BINDING


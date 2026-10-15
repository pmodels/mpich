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
	mpif_h/f2c.c \
        mpif_h/f2c_impl.c \
	mpif_h/user_proxy.c \
	mpif_h/fdebug.c \
	mpif_h/setbot.c \
	mpif_h/setbotf.f

lib_libf77_mpi_la_CPPFLAGS = $(f77_cppflags)

if BUILD_LIBF77_PMPI
mpifort_convenience_libs += lib/libf77_pmpi.la
noinst_LTLIBRARIES += lib/libf77_pmpi.la

lib_libf77_pmpi_la_SOURCES = mpif_h/fortran_binding.c

# build "pmpi_xxx_" f77 public functions
lib_libf77_pmpi_la_CPPFLAGS = $(f77_cppflags)

# build "mpi_xxx_" f77 public functions
lib_libf77_mpi_la_CPPFLAGS += -DMPICH_MPI_FROM_PMPI -DUSE_ONLY_MPI_NAMES
endif BUILD_LIBF77_PMPI

include_HEADERS += \
    mpif_h/mpi_fortran.h \
    mpif_h/mpif.h

noinst_HEADERS += \
	mpif_h/fortran_profile.h \
	mpif_h/mpi_fortimpl.h \
	mpif_h/mpifnoext.h

# setbotf.f includes mpifnoext.h
BUILT_SOURCES += mpif_h/mpifnoext.h

endif BUILD_F77_BINDING

# A version of mpif.h with no external commands, including declarations
# such as `REAL*8 MPI_WTIME, PMPI_WTIME`. Filtering on `PMPI_` covers those
# lines. It is valid in both fixed and free source form. It is used in
# BLOCK DATA (setbotf.f) and in the mpi module (use_mpi).
# NOTE: do not use extended regex to allow posix sed.
mpif_h/mpifnoext.h: mpif_h/mpif.h
	rm -f $@
	sed -e 's/^C/\!/g' -e '/EXTERNAL/d' \
	-e '/PMPI_/d' mpif_h/mpif.h > $@

CLEANFILES += mpif_h/mpifnoext.h


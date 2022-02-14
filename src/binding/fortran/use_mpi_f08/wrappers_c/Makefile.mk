##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_F08_BINDING
mpi_fc_sources += \
	src/binding/fortran/use_mpi_f08/wrappers_c/f08_cdesc.c \
	src/binding/fortran/use_mpi_f08/wrappers_c/cdesc.c \
	src/binding/fortran/use_mpi_f08/wrappers_c/comm_spawn_c.c \
	src/binding/fortran/use_mpi_f08/wrappers_c/comm_spawn_multiple_c.c \
	src/binding/fortran/use_mpi_f08/wrappers_c/utils.c

AM_CPPFLAGS += -I${main_top_srcdir}/src/binding/fortran/use_mpi_f08/wrappers_c -Isrc/binding/fortran/use_mpi_f08/wrappers_c

noinst_HEADERS += src/binding/fortran/use_mpi_f08/wrappers_c/cdesc.h

endif BUILD_F08_BINDING

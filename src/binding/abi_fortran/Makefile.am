##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS = $(MPI_ABI_H_INCLUDE)

include_HEADERS = mpi_abi_fort.h
lib_LTLIBRARIES = libmpifort_abi.la
modinc_HEADERS = $(mpi_fc_modules)

libmpifort_abi_la_SOURCES =
libmpifort_abi_la_LDFLAGS = -version-info 1:0:0
libmpifort_abi_la_LINK = $(LIBTOOL) $(AM_V_lt) --tag=FC \
			 $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) --mode=link $(FC) \
			 $(AM_FCFLAGS) $(FCFLAGS) $(libmpifort_abi_la_LDFLAGS) \
			 $(LDFLAGS) -o $@

convenience_libs =
mpi_abi_lib = -lmpi_abi

# include mpif_h/Makefile.mk
# include use_mpi/Makefile.mk
# include use_mpi_f08/Makefile.mk

noinst_LTLIBRARIES = $(convenience_libs)

libmpifort_abi_la_LIBADD = $(convenience_libs) $(mpi_abi_lib)

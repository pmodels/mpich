##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# This directory is only listed in DIST_SUBDIRS, not SUBDIRS, so its contents
# will not be built by default, but it will participate in "make distclean" and
# friends.

# override the normal compilers for the tests
CC = @TEST_CC@
F77 = @TEST_F77@

# because := is not universally available, we have to play games to use the
# user-specified LDFLAGS and OUR_LIBS env. variables (if set)
OUR_LIBS = @TEST_LIBNAME@ @MPI_LIB@

LDADD = $(OUR_LIBS)

AM_CPPFLAGS = $(ROMIO_INCLUDE)
AM_CPPFLAGS += -I$(top_builddir)/include -I$(top_srcdir)/include
AM_CPPFLAGS += -I$(top_builddir)/adio/include -I$(top_srcdir)/adio/include @mpl_includedir@
AM_CFLAGS = $(USER_CFLAGS)
AM_FFLAGS = $(USER_FFLAGS)

CTESTS = simple perf async coll_test coll_perf misc file_info excl large_array \
     atomicity noncontig i_noncontig noncontig_coll split_coll shared_fp \
     large_file psimple error status noncontig_coll2 aggregation1 aggregation2 \
     async-multiple ordered_fp hindexed external32 types_with_zeros darray_read syshints \
     fstype_prefix
FTESTS = fcoll_test fperf fmisc pfcoll_test 

check_PROGRAMS = $(CTESTS)

TESTS_ENVIRONMENT = export srcdir="$(srcdir)";

TESTS = $(check_PROGRAMS)
TEST_EXTENSIONS = .sh
LOG_COMPILER = $(builddir)/parallel_run.sh
SH_LOG_COMPILER =

EXTRA_DIST = romio_hintfile

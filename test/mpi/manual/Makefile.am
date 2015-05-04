## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/Makefile.mtest

SUBDIRS = mpi_t

noinst_PROGRAMS = singjoin testconnect testconnectserial dimsbalanced \
                  spawntest_master spawntest_child segfault

# testconnectserial would like MPICHLIBSTR to be defined as the installation
# directory of the MPI library.  This definition is not required.
noinst_HEADERS = connectstuff.h
testconnectserial_SOURCES = testconnectserial.c tchandlers.c tcutil.c 
testconnectserial_LDADD   = $(LDADD) -lm -lpthread

singjoin_LDADD = $(LDADD) @socklib@ @nslib@

CLEANFILES += test-port


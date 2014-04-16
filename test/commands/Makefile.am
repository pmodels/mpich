# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

MPICC = $(bindir)/mpicc
MPICXX = $(bindir)/mpicxx
MPIF77 = $(bindir)/mpif77
MPIFORT = $(bindir)/mpifort

## violate all automake standards and force CC and friends to be MPICC and friends
CC=$(MPICC)
CXX=$(MPICXX)
F77=$(MPIF77)
FC=$(MPIFORT)

noinst_PROGRAMS = stdiotest stdintest stdintest2 checkenv1 rtest rtestf rtestx \
		  testout

# programs with non-C source files
rtestf_SOURCES    = rtestf.F
rtestx_SOURCES    = rtestx.cxx

# this was commented out in the Makefile.sm, not sure why
testing:
#	./cmdtests

CLEANFILES = err.txt out.log out.txt a.out


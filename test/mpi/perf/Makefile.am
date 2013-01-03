## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = transp-datatype non_zero_root sendrecvl twovec dtpack \
                  allredtrace commcreatep allredtrace commcreatep timer \
                  manyrma nestvec nestvec2 indexperf

# Force all tests to be compiled with optimization 
AM_CFLAGS        = -O
# While the AM_CFLAGS should be sufficient, to make this Makefile more
# robust, indicate that these specific programs *must* be compiled with
# optimization
dtpack_CFLAGS    = -O
nestvec_CFLAGS   = -O
nestvec2_CFLAGS  = -O
indexperf_CFLAGS = -O

twovec_LDADD = -lm

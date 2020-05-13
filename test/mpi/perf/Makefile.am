##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = transp_datatype non_zero_root sendrecvl twovec dtpack \
                  allredtrace commcreatep allredtrace commcreatep timer \
                  manyrma nestvec nestvec2 indexperf gtranksperf

# Force all tests to be compiled with optimization 
AM_CFLAGS        = -O
# While the AM_CFLAGS should be sufficient, to make this Makefile more
# robust, indicate that these specific programs *must* be compiled with
# optimization
dtpack_CFLAGS    = -O
nestvec_CFLAGS   = -O
nestvec2_CFLAGS  = -O
indexperf_CFLAGS = -O

# gtranksperf uses fabs, which may require -lm
gtranksperf_LDADD = $(LDADD) -lm

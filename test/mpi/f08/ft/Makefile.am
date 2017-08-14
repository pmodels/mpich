# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f08.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f90" lines
AM_DEFAULT_SOURCE_EXT = .f90

noinst_PROGRAMS = dief08 abortf08 sendalivef08 isendalivef08 senddeadf08 recvdeadf08 isenddeadf08 \
		  irecvdeadf08 barrierf08 gatherf08 reducef08 bcastf08 scatterf08 failure_ackf08 \
		  anysourcef08 revoke_nofailf08 shrinkf08 agreef08 multi_isendalivef08 \
		  agree_shrinkf08 revoke_shrinkf08 nbccollf08

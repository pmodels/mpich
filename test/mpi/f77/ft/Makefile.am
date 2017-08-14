# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile_f77.mtest

EXTRA_DIST = testlist

# avoid having to write many "foo_SOURCES = foo.f" lines
AM_DEFAULT_SOURCE_EXT = .f

noinst_PROGRAMS = dief abortf sendalivef isendalivef senddeadf recvdeadf isenddeadf \
		  irecvdeadf barrierf gatherf reducef bcastf scatterf failure_ackf \
		  anysourcef revoke_nofailf shrinkf agreef multi_isendalivef \
		  agree_shrinkf revoke_shrinkf nbccollf

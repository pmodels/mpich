# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = die abort sendalive isendalive senddead recvdead isenddead \
		  irecvdead barrier gather reduce bcast scatter failure_ack \
		  anysource revoke_nofail shrink agree multi_isendalive \
		  agree_shrink revoke_shrink nbccoll

# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

AM_CPPFLAGS = -I$(top_builddir)/src/include -I${top_srcdir}/src/include
LDADD       = ../lib/lib@MPILIBNAME@.la -lm

# "cpi" is special, we always build it at the end of the regular "make" process
# and its main purpose is to catch missing symbol link-time errors before any
# tests are run.
noinst_PROGRAMS = cpi

# pmandel requires a separate set of socket calls (its a long story)
# and may not build on most platforms
EXTRA_PROGRAMS = pmandel pmandel_spawn pmandel_service pmandel_spaserv    \
                 pmandel_fence hellow icpi ircpi parent child srtest \
                 spawn_merge_parent spawn_merge_child1 spawn_merge_child2

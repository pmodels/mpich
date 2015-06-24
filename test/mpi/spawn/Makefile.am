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
noinst_PROGRAMS =         \
    namepub               \
    spawn1                \
    spawninfo1            \
    spawnminfo1           \
    spawn2                \
    spawnintra            \
    spawnargv             \
    spawnmanyarg          \
    spaconacc             \
    spaconacc2            \
    selfconacc            \
    spawnmult2            \
    taskmaster            \
    join                  \
    disconnect_reconnect  \
    disconnect_reconnect2 \
    disconnect_reconnect3 \
    multiple_ports        \
    multiple_ports2       \
    spaiccreate           \
    spaiccreate2          \
    disconnect            \
    disconnect2           \
    disconnect3           \
    pgroup_connect_test   \
    pgroup_intercomm_test \
    concurrent_spawns     \
    spawn-rootargs

join_LDADD = $(LDADD) @socklib@ @nslib@

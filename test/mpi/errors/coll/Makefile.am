##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

EXTRA_DIST = testlist

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS = noalias noalias2 noalias3 rerr bcastlength reduce_local nb_noalias nb_rerr ibcastlength reducelength ireducelength allreducelength iallreducelength reduceop ireduceop gatherlength igatherlength scatterlength iscatterlength allgatherlength iallgatherlength alltoalllength

ibcastlength_SOURCES = bcastlength.c
ibcastlength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

ireduceop_SOURCES = reduceop.c
ireduceop_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

ireducelength_SOURCES = reducelength.c
ireducelength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

iallreducelength_SOURCES = allreducelength.c
iallreducelength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

igatherlength_SOURCES = gatherlength.c
igatherlength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

iscatterlength_SOURCES = scatterlength.c
iscatterlength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

iallgatherlength_SOURCES = allgatherlength.c
iallgatherlength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

alltoalllength_SOURCES = alltoalllength.c
alltoalllength_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nb_noalias_SOURCES = noalias.c
nb_noalias_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nb_rerr_SOURCES = rerr.c
nb_rerr_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)


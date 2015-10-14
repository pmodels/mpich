# -*- Mode: Makefile; -*-
# vim: set ft=automake :
#
# (C) 2011 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

include $(top_srcdir)/Makefile.mtest

EXTRA_DIST = testlist.in

## for all programs that are just built from the single corresponding source
## file, we don't need per-target _SOURCES rules, automake will infer them
## correctly
noinst_PROGRAMS =      \
    allgather2         \
    allgather3         \
    allgatherv2        \
    allgatherv3        \
    allgatherv4        \
    allgather_struct   \
    allred             \
    allred2            \
    allred3            \
    allred4            \
    allred5            \
    allred6            \
    allredmany         \
    alltoall1          \
    alltoallv          \
    alltoallv0         \
    alltoallw1         \
    alltoallw2         \
    alltoallw_zeros    \
    bcast_full         \
    bcast_min_datatypes \
    bcast_comm_world   \
    bcasttest          \
    bcastzerotype      \
    coll2              \
    coll3              \
    coll4              \
    coll5              \
    coll6              \
    coll7              \
    coll8              \
    coll9              \
    coll10             \
    coll11             \
    coll12             \
    coll13             \
    exscan             \
    exscan2            \
    gather             \
    gather2            \
    gather_big         \
    iallred            \
    ibarrier           \
    icallgather        \
    icallgatherv       \
    icallreduce        \
    icalltoall         \
    icalltoallv        \
    icalltoallw        \
    icbarrier          \
    icbcast            \
    icgather           \
    icgatherv          \
    icreduce           \
    icscatter          \
    icscatterv         \
    longuser           \
    nonblocking        \
    nonblocking2       \
    nonblocking3       \
    op_commutative     \
    opband             \
    opbor              \
    opbxor             \
    opland             \
    oplor              \
    oplxor             \
    opmax              \
    opmaxloc           \
    opmin              \
    opminloc           \
    opprod             \
    opsum              \
    red3               \
    red4               \
    red_scat_block     \
    red_scat_block2    \
    redscat            \
    redscat2           \
    redscat3           \
    redscatblk3        \
    redscatinter       \
    redscatbkinter     \
    reduce             \
    reduce_local       \
    scantst            \
    scatter2           \
    scatter3           \
    scattern           \
    scatterv           \
    uoplong            \
    nballtoall1        \
    nbredscat          \
    nbredscat3	       \
    nbredscatinter     \
    nbcoll2            \
    nbicbarrier        \
    nbicallgather      \
    nbicallgatherv     \
    nbicallreduce      \
    nbicbcast          \
    nbicalltoall       \
    nbicalltoallw      \
    nbicalltoallv      \
    nbicgather         \
    nbicgatherv        \
    nbicreduce         \
    nbicscatter        \
    nbicscatterv
allgatherv4_LDADD = $(LDADD) -lm

bcast_full_SOURCES = bcast.c

bcast_min_datatypes_CPPFLAGS = -DBCAST_MIN_DATATYPES_ONLY $(AM_CPPFLAGS)
bcast_min_datatypes_SOURCES = bcast.c

bcast_comm_world_CPPFLAGS = -DBCAST_COMM_WORLD_ONLY $(AM_CPPFLAGS)
bcast_comm_world_SOURCES = bcast.c

# Generate tests of the nonblocking collectives by reusing the blocking tests.
nballtoall1_SOURCES = alltoall1.c
nballtoall1_CPPFLAGS  = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbredscat_SOURCES   = redscat.c
nbredscat_CPPFLAGS     = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbredscat3_SOURCES   = redscat3.c
nbredscat3_CPPFLAGS     = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbredscatinter_SOURCES   = redscatinter.c
nbredscatinter_CPPFLAGS     = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbcoll2_SOURCES = coll2.c
nbcoll2_CPPFLAGS   = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicbarrier_SOURCES = icbarrier.c
nbicbarrier_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicallgather_SOURCES = icallgather.c
nbicallgather_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbicallgatherv_SOURCES = icallgatherv.c
nbicallgatherv_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicallreduce_SOURCES = icallreduce.c
nbicallreduce_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicalltoall_SOURCES = icalltoall.c
nbicalltoall_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbicalltoallv_SOURCES = icalltoallv.c
nbicalltoallv_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbicalltoallw_SOURCES = icalltoallw.c
nbicalltoallw_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicbcast_SOURCES = icbcast.c
nbicbcast_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicgather_SOURCES = icgather.c
nbicgather_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbicgatherv_SOURCES = icgatherv.c
nbicgatherv_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicreduce_SOURCES = icreduce.c
nbicreduce_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

nbicscatter_SOURCES = icscatter.c
nbicscatter_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)
nbicscatterv_SOURCES = icscatterv.c
nbicscatterv_CPPFLAGS = -DUSE_MTEST_NBC $(AM_CPPFLAGS)

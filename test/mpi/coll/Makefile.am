##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest

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
    allred_derived     \
    allred_float       \
    allredmany         \
    alltoall1          \
    alltoallv          \
    alltoallv0         \
    alltoallw1         \
    alltoallw2         \
    alltoallw_zeros    \
    bcasttest          \
    bcastzerotype      \
    coll2              \
    gatherv            \
    coll4              \
    coll5              \
    coll6              \
    coll7              \
    coll8              \
    coll9              \
    coll10             \
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
    op_coll            \
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
    p_allred           \
    p_bcast            \
    p_bcast2           \
    p_red              \
    p_alltoall         \
    p_allgather        \
    p_allgatherv       \
    p_red_scat_block   \
    p_redscat          \
    p_scan             \
    p_gather           \
    p_gatherv          \
    p_scatter          \
    p_scatterv         \
    p_alltoallv        \
    p_alltoallw        \
    p_exscan           \
    p_barrier          \
    p_neighb_allgather \
    p_neighb_allgatherv\
    p_neighb_alltoall  \
    p_neighb_alltoallv \
    p_neighb_alltoallw \
    p_order            \
    p_inactive         \
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
    uoplong_large      \
    uop_equal	       \
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
    nbicscatterv       \
    neighb_allgather   \
    neighb_allgatherv  \
    neighb_alltoall    \
    neighb_alltoallv   \
    neighb_alltoallw   \
    neighb_alltoallw2  \
    bcast              \
    bcast_comm_world_only \
    coll_large	       \
    ring_neighbor_alltoall

allgatherv4_LDADD = $(LDADD) -lm

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

bcast_CPPFLAGS = $(AM_CPPFLAGS)
bcast_SOURCES = bcast.c

bcast_comm_world_only_CPPFLAGS = -DBCAST_COMM_WORLD_ONLY $(AM_CPPFLAGS)
bcast_comm_world_only_SOURCES = bcast.c

uoplong_large_SOURCES = uoplong.c
uoplong_large_CPPFLAGS = -DOP_LARGE $(AM_CPPFLAGS)


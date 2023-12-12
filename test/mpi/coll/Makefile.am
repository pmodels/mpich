##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_single.mtest
include Makefile_mpitests.mk

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
    alltoall_gpu       \
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
    neighb_allgather   \
    neighb_allgatherv  \
    neighb_alltoall    \
    neighb_alltoallv   \
    neighb_alltoallw   \
    neighb_alltoallw2  \
    bcast              \
    bcast_gpu          \
    coll_large	       \
    ring_neighbor_alltoall

uoplong_large_SOURCES = uoplong.c
uoplong_large_CPPFLAGS = -DOP_LARGE $(AM_CPPFLAGS)


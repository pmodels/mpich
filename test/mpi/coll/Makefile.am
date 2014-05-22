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
noinst_PROGRAMS =      \
    allgather2         \
    allgather3         \
    allgatherv2        \
    allgatherv3        \
    allgatherv4        \
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
    bcast2             \
    bcast3             \
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
    nonblocking4       \
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
    nbcoll2            

allgatherv4_LDADD = $(LDADD) -lm

# Create tests of the nonblocking collectives by reusing the blocking tests,
# using the MPI profiling interface to intercept the blocking call and
# replace it with a MPI_Ixxx/MPI_Wait pair.
nballtoall1_SOURCES = alltoall1.c
nballtoall1_LDADD   = $(LDADD) ../util/nbc_pmpi_adapter.$(OBJEXT)
nbredscat_SOURCES   = redscat.c
nbredscat_LDADD     = $(LDADD) ../util/nbc_pmpi_adapter.$(OBJEXT)
nbredscat3_SOURCES   = redscat3.c
nbredscat3_LDADD     = $(LDADD) ../util/nbc_pmpi_adapter.$(OBJEXT)
nbredscatinter_SOURCES   = redscatinter.c
nbredscatinter_LDADD     = $(LDADD) ../util/nbc_pmpi_adapter.$(OBJEXT)
nbcoll2_SOURCES = coll2.c
nbcoll2_LDADD   = $(LDADD) ../util/nbc_pmpi_adapter.$(OBJEXT)

# This is here instead of Makefile.mtest because it is only used in this 
# set of tests
$(top_builddir)/util/nbc_pmpi_adapter.$(OBJEXT): $(top_srcdir)/util/nbc_pmpi_adapter.c
	(cd $(top_builddir)/util && $(MAKE) nbc_pmpi_adapter.$(OBJEXT))

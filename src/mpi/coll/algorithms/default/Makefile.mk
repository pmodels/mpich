## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
AM_CPPFLAGS += -I$(top_srcdir)/src/mpi/coll/algorithms/default

mpi_core_sources +=                     				\
    src/mpi/coll/algorithms/default/allgather.c       	\
    src/mpi/coll/algorithms/default/allgatherv.c       	\
    src/mpi/coll/algorithms/default/allreduce.c       	\
    src/mpi/coll/algorithms/default/alltoall.c       	\
    src/mpi/coll/algorithms/default/alltoallv.c       	\
    src/mpi/coll/algorithms/default/alltoallw.c       	\
    src/mpi/coll/algorithms/default/barrier.c       	\
    src/mpi/coll/algorithms/default/bcast.c		       	\
    src/mpi/coll/algorithms/default/gather.c    	   	\
    src/mpi/coll/algorithms/default/gatherv.c    	   	\
    src/mpi/coll/algorithms/default/red_scat_block.c   	\
    src/mpi/coll/algorithms/default/red_scat.c  	 	\
    src/mpi/coll/algorithms/default/reduce.c   			\
    src/mpi/coll/algorithms/default/scatter.c  			\
    src/mpi/coll/algorithms/default/scatterv.c

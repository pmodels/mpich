#!/bin/sh
# 
# This script will safely convert generated functions that begin with "MPIX_" or
# "MPITX_" into non-extension, MPI Forum sanctioned functions that start with
# "MPI_" and "MPIT_" respectively.
# 
# Script callers should set FIXUP_MPI_H="src/include/mpi.h.in src/mpi/errhan/errnames.txt"
# or similar if they want prototypes to also be rewritten.

set -x

perl -p -i -e 's/\b(P)?MPIX_Ibarrier\b/$1MPI_Ibarrier/g; s/\*\*mpix_ibarrier\b/**mpi_ibarrier/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ibarrier.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ibcast\b/$1MPI_Ibcast/g; s/\*\*mpix_ibcast\b/**mpi_ibcast/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ibcast.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Igather\b/$1MPI_Igather/g; s/\*\*mpix_igather\b/**mpi_igather/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/igather.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Igatherv\b/$1MPI_Igatherv/g; s/\*\*mpix_igatherv\b/**mpi_igatherv/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/igatherv.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iscatter\b/$1MPI_Iscatter/g; s/\*\*mpix_iscatter\b/**mpi_iscatter/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iscatter.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iscatterv\b/$1MPI_Iscatterv/g; s/\*\*mpix_iscatterv\b/**mpi_iscatterv/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iscatterv.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iallgather\b/$1MPI_Iallgather/g; s/\*\*mpix_iallgather\b/**mpi_iallgather/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iallgather.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iallgatherv\b/$1MPI_Iallgatherv/g; s/\*\*mpix_iallgatherv\b/**mpi_iallgatherv/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iallgatherv.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ialltoall\b/$1MPI_Ialltoall/g; s/\*\*mpix_ialltoall\b/**mpi_ialltoall/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ialltoall.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ialltoallv\b/$1MPI_Ialltoallv/g; s/\*\*mpix_ialltoallv\b/**mpi_ialltoallv/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ialltoallv.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ialltoallw\b/$1MPI_Ialltoallw/g; s/\*\*mpix_ialltoallw\b/**mpi_ialltoallw/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ialltoallw.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ireduce\b/$1MPI_Ireduce/g; s/\*\*mpix_ireduce\b/**mpi_ireduce/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ireduce.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iallreduce\b/$1MPI_Iallreduce/g; s/\*\*mpix_iallreduce\b/**mpi_iallreduce/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iallreduce.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ireduce_scatter\b/$1MPI_Ireduce_scatter/g; s/\*\*mpix_ireduce_scatter\b/**mpi_ireduce_scatter/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ired_scat.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Ireduce_scatter_block\b/$1MPI_Ireduce_scatter_block/g; s/\*\*mpix_ireduce_scatter_block\b/**mpi_ireduce_scatter_block/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/ired_scat_block.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iscan\b/$1MPI_Iscan/g; s/\*\*mpix_iscan\b/**mpi_iscan/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iscan.c' $FIXUP_MPI_H
perl -p -i -e 's/\b(P)?MPIX_Iexscan\b/$1MPI_Iexscan/g; s/\*\*mpix_iexscan\b/**mpi_iexscan/g; s/\bMPID_STATE_MPI(T)?X_/MPID_STATE_MPI${1}_/g' \
    'src/mpi/coll/iexscan.c' $FIXUP_MPI_H

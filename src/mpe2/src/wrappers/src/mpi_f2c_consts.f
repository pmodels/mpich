! 
!  (C) 2001 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      subroutine fsub_mpi_fconsts( F_MPI_STATUS_SIZE, itrue, ifalse )
      implicit none
      include "mpif.h"
      integer F_MPI_STATUS_SIZE
      logical itrue, ifalse
      integer ierr

      F_MPI_STATUS_SIZE = MPI_STATUS_SIZE
      itrue   = .TRUE.
      ifalse  = .FALSE.
      call csub_mpi_in_place( MPI_IN_PLACE )
      call csub_mpi_status_ignore( MPI_STATUS_IGNORE )
      call csub_mpi_statuses_ignore( MPI_STATUSES_IGNORE )
      end

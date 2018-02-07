C -*- Mode: Fortran; -*- 

C
C This test ensures that MPI_Comm_revoke works when a process failure 
C has not occurred yet.
C
      program main
      include "mpif.h"
      integer rank, size
      integer rc, ec, ierr
      character*511 error
      integer world_dup

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                             ierr)

      call MPI_Comm_dup(MPI_COMM_WORLD, world_dup, ierr)

      if (rank .eq. 1) call MPIX_Comm_revoke(world_dup, ierr)

      call MPI_Barrier(world_dup, rc)
      call MPI_Error_class(rc, ec, ierr)
      if (ec .ne. MPIX_ERR_REVOKED) then
         call MPI_Error_string(ec, error, size, ierr)
         write(*,*) "[", rank, 
     &          "] MPI_Barrier should have returned MPIX_ERR_REVOKED (",
     &          MPIX_ERR_REVOKED, "), but it actually returned:", ec
         write(*,*) error
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_free(world_dup, ierr)

      if (rank .eq. 0) write(*,*) " No errors"

      call MPI_Finalize(ierr)

      end program

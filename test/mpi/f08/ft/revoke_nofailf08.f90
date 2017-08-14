! -*- Mode: Fortran; -*- 

!
! This test ensures that MPI_Comm_revoke works when a process failure 
! has not occurred yet.
!
      program main
      use mpi_f08
      implicit none
      integer rank, size
      integer rc, ec, ierr
      character(len=MPI_MAX_ERROR_STRING) error
      type(MPI_Comm) world_dup

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      call MPI_Comm_dup(MPI_COMM_WORLD, world_dup, ierr)

      if (rank .eq. 1) call MPIX_Comm_revoke(comm=world_dup, ierror=ierr)

      call MPI_Barrier(world_dup, rc)
      call MPI_Error_class(rc, ec, ierr)
      if (ec .ne. MPIX_ERR_REVOKED) then
         call MPI_Error_string(ec, error, size, ierr)
         write(*,*) "[", rank, &
     &          "] MPI_Barrier should have returned MPIX_ERR_REVOKED (", &
     &          MPIX_ERR_REVOKED, "), but it actually returned:", ec
         write(*,*) error
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_free(world_dup, ierr)

      if (rank .eq. 0) write(*,*) " No errors"

      call MPI_Finalize(ierr)

      end program

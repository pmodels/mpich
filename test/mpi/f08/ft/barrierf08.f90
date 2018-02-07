! -*- Mode: Fortran; -*- 

!
!  This test attempts collective communication after a process in
!  the communicator has failed. Since all processes contribute to
!  the result of the operation, all process will receive an error.
!

      program main
      use mpi_f08
      implicit none

      integer rank, size
      integer err, errclass
      integer ierr

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      call MPI_Barrier(MPI_COMM_WORLD, err)

      if (rank .eq. 0) then
         call MPI_Error_class(err, errclass, ierr)
         if (errclass .eq. MPIX_ERR_PROC_FAILED) then
            write(*,*) " No Errors"
         else
            write(*,*) "Wrong error code (", errclass, ") returned. " &
     &            // "Expected MPIX_ERR_PROC_FAILED"
         end if
      end if

      call MPI_Finalize(ierr)

      end program

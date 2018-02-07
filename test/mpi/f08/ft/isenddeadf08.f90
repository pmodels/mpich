! -*- Mode: Fortran; -*- 

!
! This test attempts to MPI_Isend with the destination being a dead
! process. The communication should succeed or report an error. It must
! not block indefinitely.
!
      program main
      use mpi_f08
      implicit none
      integer rank, size, err, errclass, ierr
      character(len=100000) buf
      type(MPI_Request) request
      type(MPI_Status) status

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                                ierr)
         call MPI_Isend(buf, 100000, MPI_CHARACTER, 1, 0, &
     &                  MPI_COMM_WORLD, request, err)
         if (err .ne. 0) write(*,*) "MPI_Isend returned error"

         call MPI_Wait(request, status, err)
         call MPI_Error_class(err, errclass, ierr)
         if ((err .ne. 0) .and. &
     &       (errclass .ne. MPIX_ERR_PROC_FAILED)) then
            write (*,*)  "Wrong error code (", errclass, &
     &                   ") returned. Expected MPIX_ERR_PROC_FAILED"
         else
            write(*,*) " No Errors"
         end if
      end if

      call MPI_Finalize(ierr)

      end program

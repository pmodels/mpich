! -*- Mode: Fortran; -*- 

!
!  This test attempts communication between 2 running processes
!  after another process has failed. The communication should complete
!  successfully.
!
      program main
      use mpi_f08
      implicit none
      integer rank, size, err, ierr
      character(len=10) buf
      type(MPI_Status) status
      type(MPI_Request) request

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)
      if (size .lt. 3) then
         write(*,*) "Must run with at least 3 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Isend("No Errors ", 10, MPI_CHAR, 2, 0, &
     &                  MPI_COMM_WORLD, request, err)
         call MPI_Wait(request, status, ierr)
         if (err + ierr .ne. 0) then 
            write(*,*) "An error occurred during the send operation"
         end if
      end if

      if (rank == 2) then
         call MPI_Irecv(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &
     &                  request, err)
         call MPI_Wait(request, status, err)
         if (err .ne. 0) then
            write(*,*) "An error occurred during the recv operation"
         else
            write(*,*) buf
         end if
      end if

      call MPI_Finalize(ierr)

      end program

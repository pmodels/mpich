C -*- Mode: Fortran; -*- 

C
C  This test attempts communication between 2 running processes
C  after another process has failed. The communication should complete
C  successfully.
C
      program main
      include "mpif.h"
      integer rank, size, err, ierr
      character*10 buf
      integer request

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                             ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 4 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Issend("No Errors ", 10, MPI_CHARACTER, 3, 0,
     &                   MPI_COMM_WORLD, request, err)
         call MPI_Wait(request, MPI_STATUS_IGNORE, ierr)
         if (err + ierr .ne. 0) then
            write(*,*) "An error occurred during the send operation"
         end if
      end if

      if (rank .eq. 3) then
         call MPI_Irecv(buf, 10, MPI_CHARACTER, 0, 0, MPI_COMM_WORLD,
     &                  request, err)
         call MPI_Wait(request, MPI_STATUS_IGNORE, ierr)
         if (err + ierr .ne. 0) then
             write(*,*) "An error occurred during the recv operation"
         end if
      end if

      if (rank .eq. 3) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Issend("No Errors ", 10, MPI_CHARACTER, 2, 0,
     &                   MPI_COMM_WORLD, request, err)
         call MPI_Wait(request, MPI_STATUS_IGNORE, ierr)
         if (err + ierr .ne. 0) then
            write (*,*)  "An error occurred during the send operation"
         end if
      end if

      if (rank .eq. 2) then
         call MPI_Irecv(buf, 10, MPI_CHARACTER, 0, 0, MPI_COMM_WORLD,
     &                  request, err)
         call MPI_Wait(request, MPI_STATUS_IGNORE, ierr)
         if (err+ierr .ne. 0) then
            write (*,*) "An error occurred during the recv operation"
         else
            write(*,*) buf
         end if
      end if

      if (rank .eq. 2) error stop "FORTRAN STOP"
      
      call MPI_Finalize(ierr)

      end program

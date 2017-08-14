C -*- Mode: Fortran; -*- 

      subroutine myerrhanfunc(communicator, error_code)
      include 'mpif.h'
      integer communicator, error_code
      integer new_comm
      integer ierr
      integer comm_all
      common /myerrhand/ comm_all

      call MPIX_Comm_revoke(comm_all, ierr)
      call MPIX_Comm_shrink(comm_all, new_comm, ierr)

      call MPI_Comm_free(comm_all, ierr)

      comm_all = new_comm
      end subroutine

      program main
      include 'mpif.h'
      integer rank, size, i, ierr
      integer sum, val
      integer errs
      integer comm_all
      common /myerrhand/ comm_all
      integer errhandler
      external myerrhanfunc
CF90  interface 
CF90     subroutine myerrhanfunc(communicator, error_code)
CF90        integer communicator, error_code
CF90     end subroutine
CF90  end interface

      sum = 0
      val = 1
      errs = 0

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 4 processes."
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_dup(MPI_COMM_WORLD, comm_all, ierr)

      call MPI_Comm_create_errhandler(myerrhanfunc, errhandler, ierr)
      call MPI_Comm_set_errhandler(comm_all, errhandler, ierr)

      do i = 0, 9
         call MPI_Comm_size(comm_all, size, ierr)
         sum = 0;
         if ((i .eq. 5) .and. (rank .eq. 1)) then
            error stop "FORTRAN STOP"
         else if (i .ne. 5) then
            call MPI_Allreduce(val, sum, 1, MPI_INTEGER, MPI_SUM, 
     &                         comm_all, ierr)
            if ((sum .ne. size) .and. (rank .eq. 0)) then
               errs = errs + 1
               write(*,*) "Incorrect answer:", sum, "!=", size
            end if
         end if
      end do

      if ((0 .eq. rank) .and. (errs .ne. 0)) then
         write(*,*) " Found", errs, "errors"
      else if (0 .eq. rank) then
         write(*,*) " No errors"
      end if

      call MPI_Comm_free(comm_all, ierr)
      call MPI_Errhandler_free(errhandler, ierr)

      call MPI_Finalize(ierr)

      end program

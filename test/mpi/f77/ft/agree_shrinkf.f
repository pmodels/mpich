C -*- Mode: Fortran; -*- 

      program main
      include 'mpif.h'

      integer rank, size, rc, ec, errs, flag, dup, shrunk, ierr

      errs = 0
      flag = 1

      call MPI_Init(ierr)
      call MPI_Comm_dup(MPI_COMM_WORLD, dup, ierr)
      call MPI_Comm_rank(dup, rank, ierr)
      call MPI_Comm_size(dup, size, ierr)
      call MPI_Comm_set_errhandler(dup, MPI_ERRORS_RETURN, ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 4 processes"
         call MPI_Abort(dup, 1, ierr)
      end if

      if (2 .eq. rank) error stop "FORTRAN STOP"

      call MPIX_Comm_agree(dup, flag, rc)
      if (MPI_SUCCESS .eq. rc) then
        call MPI_Error_class(rc, ec, ierr)
        write(*,*) "[", rank, 
     &    "] Expected MPIX_ERR_PROC_FAILED after agree. Received: ", ec
        errs = errs + 1
        call MPI_Abort(dup, 1, ierr)
      end if

      call MPIX_Comm_shrink(dup, shrunk, rc)
      if (MPI_SUCCESS .ne. rc) then
        call MPI_Error_class(rc, ec, ierr)
        write(*,*) "[", rank, 
     &    "] Expected MPI_SUCCESS after shrink. Received: ", ec
        errs = errs + 1
        call MPI_Abort(dup, 1, ierr)
      end if

      call MPIX_Comm_agree(shrunk, flag, rc)
      if (MPI_SUCCESS .ne. rc) then
        call MPI_Error_class(rc, ec, ierr)
        write(*,*) "[", rank,
     &    "] Expected MPI_SUCCESS after agree. Received: ", ec
        errs = errs + 1
        call MPI_Abort(dup, 1, ierr)
      end if

      call MPI_Comm_free(shrunk, ierr)
      call MPI_Comm_free(dup, ierr)

      if (rank .eq. 0) then
         if (errs .gt. 0) then
            write(*,*) " Found ", errs, " errors"
         else
            write(*,*) " No Errors"
         end if
      end if

      call MPI_Finalize(ierr)

      end program

! -*- Mode: Fortran; -*- 

      program main
      use mpi_f08
      implicit none
      integer rank, size, rc, errclass, errs, flag, ierr

      errs = 0
      flag = 1

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 4 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (2 .eq. rank) error stop "FORTRAN STOP"

      if (0 .eq. rank) flag = 0

      call MPIX_Comm_agree(comm=MPI_COMM_WORLD, flag=flag, ierror=rc)
      call MPI_Error_class(rc, errclass, ierr)
      if (errclass .ne. MPIX_ERR_PROC_FAILED) then
         write(*,*) "[", rank, &
     &     "] Expected MPIX_ERR_PROC_FAILED after agree. Received: ", &
     &     errclass
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         errs = errs + 1
      else if (0 .ne. flag) then
         write(*,*) "[",rank,"] Expected flag to be 0. Received: ", flag
         errs = errs + 1
      end if

      call MPIX_Comm_failure_ack(comm=MPI_COMM_WORLD, ierror=ierr);

      if (0 .eq. rank) then
         flag = 0
      else
         flag = 1
      end if
      call MPIX_Comm_agree(comm=MPI_COMM_WORLD, flag=flag, ierror=rc)
      call MPI_Error_class(rc, errclass, ierr)
      if (MPI_SUCCESS .ne. rc) then
         write(*,*) "[", rank, &
     &     "] Expected MPI_SUCCESS after agree. Received: ", errclass
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         errs = errs + 1
      else if (0 .ne. flag) then
         write(*,*) "[",rank,"] Expected flag to be 0. Received: ", flag
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         errs = errs + 1
      end if

      call MPI_Finalize(ierr)

      if (rank .eq. 0) then
         if (errs .eq. 0) then
            write(*,*) " No Errors"
         else
            write(*,*) " Found ", errs, " errors"
         end if
      end if

      end program

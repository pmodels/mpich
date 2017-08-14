! -*- Mode: Fortran; -*- 

!
!  This test checks if a non-blocking collective failing impacts the
!  result of another operation going on at the same time.
!
      program main
      use mpi_f08
      implicit none

      integer rank, size
      integer err, ierr
      integer excl(1), ec, ec2
      type(MPI_Comm) small_comm
      type(MPI_Group) orig_grp, small_grp
      type(MPI_Request) req
      type(MPI_Status) status

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      call MPI_Comm_group(MPI_COMM_WORLD, orig_grp, ierr)
      call MPI_Group_size(orig_grp, excl(1), ierr)
      excl = excl - 1
      call MPI_Group_excl(orig_grp, 1, excl, small_grp, ierr)
      call MPI_Comm_create_group(MPI_COMM_WORLD, small_grp, 0, &
     &                           small_comm, ierr)
      call MPI_Group_free(orig_grp, ierr)
      call MPI_Group_free(small_grp, ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. excl(1)) error stop "FORTRAN STOP"

      call MPI_Ibarrier(MPI_COMM_WORLD, req, ierr)

      call MPI_Barrier(small_comm, err)
      if (err .ne. MPI_SUCCESS) then
         call MPI_Error_class(err, ec, ierr)
         write(*,*) "Result != MPI_SUCCESS:", ec
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Wait(req, status, err)
      if (err .eq. MPI_SUCCESS) then
         call MPI_Error_class(err, ec, ierr)
         call MPI_Error_class(status%MPI_ERROR, ec2, ierr)
         write(*,*) "Result != MPIX_ERR_PROC_FAILED:", ec, "Status:",ec2
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_free(small_comm, ierr)

      if (rank .eq. 0) write(*,*) " No Errors"

      call MPI_Finalize(ierr)

      end program

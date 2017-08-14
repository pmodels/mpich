! -*- Mode: Fortran; -*- 

! 
! This test makes sure that after a failure, the correct group of failed
! processes is returned from MPIX_Comm_failure_ack/get_acked.
!
      program main
      use mpi_f08
      implicit none
      integer rank, size, err, result, i
      integer ec, ierr
      character(len=10) buf
      character(len=MPI_MAX_ERROR_STRING) error
      type(MPI_Group) failed_grp, one_grp, world_grp
      integer one(1)
      integer world_ranks(3)
      integer failed_ranks(3)
      type(MPI_Status) status

      buf = " No errors"
      one(1) = 1
      world_ranks(1) = 0
      world_ranks(2) = 1
      world_ranks(3) = 2

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 3) then
         write(0,*) "Must run with at least 3 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Recv(buf, 10, MPI_CHARACTER, 1, 0, MPI_COMM_WORLD, &
     &                 status, err)
         if (MPI_SUCCESS .eq. err) then
            write(0,*) "Expected a failure for receive from rank 1"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPIX_Comm_failure_ack(comm=MPI_COMM_WORLD, ierror=err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            call MPI_Error_string(err, error, size, ierr)
            write(0,*) "MPIX_Comm_failure_ack returned an error:", ec
            write(0,*) error
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if
         call MPIX_Comm_failure_get_acked(comm=MPI_COMM_WORLD, failedgrp=failed_grp, ierror=err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            call MPI_Error_string(err, error, size, ierr)
            write(0,*) "MPIX_Comm_failure_get_acked returned an error:", &
     &                 ec
            write (0,*) error
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPI_Comm_group(MPI_COMM_WORLD, world_grp, ierr)
         call MPI_Group_incl(world_grp, 1, one, one_grp, ierr)
         call MPI_Group_compare(one_grp, failed_grp, result, ierr)
         if (MPI_IDENT .ne. result) then
            write(0,*) "First failed group contains incorrect processes"
            call MPI_Group_size(failed_grp, size, ierr)
            call MPI_Group_translate_ranks(failed_grp, size, &
     &                       world_ranks, world_grp, failed_ranks, ierr)
            do i = 1, size
               write(0,*) "DEAD:", failed_ranks(i)
            end do
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if
         call MPI_Group_free(failed_grp, ierr)

         call MPI_Recv(buf, 10, MPI_CHARACTER, 2, 0, MPI_COMM_WORLD, &
     &                 status, err)
         if (MPI_SUCCESS .ne. err) then
            write (0,*) "First receive failed"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if
         call MPI_Recv(buf, 10, MPI_CHARACTER, 2, 0, MPI_COMM_WORLD, &
     &                 status, err)
         if (MPI_SUCCESS .eq. err) then
            write(0,*) "Expected a failure for receive from rank 2"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPIX_Comm_failure_get_acked(comm=MPI_COMM_WORLD, failedgrp=failed_grp, ierror=err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            call MPI_Error_string(err, error, size, ierr)
            write(0,*) "MPIX_Comm_failure_get_acked returned an error:", &
     &                 ec
            write(0,*) error
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPI_Group_compare(one_grp, failed_grp, result, ierr)
         if (MPI_IDENT .ne. result) then
            write(0,*)"Second failed group contains incorrect processes"
            call MPI_Group_size(failed_grp, size, ierr)
            call MPI_Group_translate_ranks(failed_grp, size, &
     &                       world_ranks, world_grp, failed_ranks, ierr)
            do i = 1, size
               write(*,*) "DEAD:", failed_ranks(i)
            end do
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         write(*,*) " No errors"
      else if (rank .eq. 2) then
         call MPI_Ssend(buf, 10, MPI_CHARACTER, 0, 0, MPI_COMM_WORLD, &
     &                  ierr)

         error stop "FORTRAN STOP"
      end if

      call MPI_Group_free(failed_grp, ierr)
      call MPI_Group_free(one_grp, ierr)
      call MPI_Group_free(world_grp, ierr)
      call MPI_Finalize(ierr)
      end program

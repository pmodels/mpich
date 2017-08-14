C -*- Mode: Fortran; -*- 

      program main
      include 'mpif.h'

      integer rank, size, err, ec, ierr
      character*10 buf
      integer request
      integer status(MPI_STATUS_SIZE)

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 3) then
         write(*,*) "Must run with at least 3 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                             ierr)

      if (rank .eq. 1) error stop "FORTRAN STOP"

C      Make sure ANY_SOURCE returns correctly after a failure
      if (rank .eq. 0) then
         call MPI_Recv(buf, 10, MPI_CHARACTER, MPI_ANY_SOURCE, 0,
     &                 MPI_COMM_WORLD, MPI_STATUS_IGNORE, err)
         call MPI_Error_class(err, ec, ierr)
         if (MPIX_ERR_PROC_FAILED .ne. ec) then
            write(*,*) "Expected MPIX_ERR_PROC_FAILED for receive from "
     &            // "ANY_SOURCE: ", ec
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

C        Make sure that new ANY_SOURCE operations don't work yet
         call MPI_Irecv(buf, 10, MPI_CHARACTER, MPI_ANY_SOURCE, 0,
     &                  MPI_COMM_WORLD, request, err)
         if (request .eq. MPI_REQUEST_NULL) then
            write(*,*) "Request for ANY_SOURCE receive is NULL"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if
         call MPI_Error_class(err, ec, ierr)
         if (ec .ne. MPI_SUCCESS .and.
     &      ec .ne. MPIX_ERR_PROC_FAILED_PENDING) then
            write(*,*)
     &         "Expected SUCCESS or MPIX_ERR_PROC_FAILED_PENDING: ", ec
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPI_Wait(request, status, err)
         call MPI_Error_class(err, ec, ierr)
         if (MPIX_ERR_PROC_FAILED_PENDING .ne. ec) then
            write(*,*) "Expected a MPIX_ERR_PROC_FAILED_PENDING (",
     &               MPIX_ERR_PROC_FAILED_PENDING,
     &               " for receive from ANY_SOURCE: ", ec
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPI_Send(NULL, 0, MPI_INTEGER, 2, 0, MPI_COMM_WORLD, err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            write(*,*) "MPI_SEND failed: ", ec
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

C        Make sure that ANY_SOURCE works after failures are acknowledged
         call MPIX_Comm_failure_ack(MPI_COMM_WORLD, ierr)
         call MPI_Wait(request, status, err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            write(*,*)"Unexpected failure after acknowledged failure (",
     &               ec, ")"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         write(*,*) buf
      else if (rank .eq. 2) then
         buf = "No errors"
         call MPI_Recv(NULL, 0, MPI_INTEGER, 0, 0, MPI_COMM_WORLD,
     &                 MPI_STATUS_IGNORE, err)
         if (MPI_SUCCESS .ne. err) then
             call MPI_Error_class(err, ec, ierr)
             write(*,*) "MPI_RECV failed: ", ec
             call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if

         call MPI_Send(buf, 10, MPI_CHARACTER, 0, 0, MPI_COMM_WORLD,err)
         if (MPI_SUCCESS .ne. err) then
            call MPI_Error_class(err, ec, ierr)
            write(*,*) "Unexpected failure from MPI_Send (", ec, ")"
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         end if
      end if

      call MPI_Finalize(ierr)

      end program

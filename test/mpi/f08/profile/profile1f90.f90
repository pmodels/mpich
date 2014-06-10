! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi_f08
       integer ierr
       integer smsg(3), rmsg(3), toterrs, wsize, wrank
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount

       toterrs = 0
       call mpi_init( ierr )
       call init_counts()
       call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
       call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )

       if (wrank .eq. 0) then
           smsg(1) = 3
           call mpi_send( smsg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, ierr )
       else if (wrank .eq. 1) then
          call mpi_recv( rmsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &
      &         MPI_STATUS_IGNORE, ierr )
          if (rmsg(1) .ne. 3) then
             toterrs = toterrs + 1
             print *, "Unexpected received value ", rmsg(1)
          endif
       endif
!
!     check that we used the profiling versions of the routines
       toterrs = 0
       if (wrank .eq. 0) then
          if (calls.ne.1) then
             toterrs = toterrs + 1
             print *, "Sender calls is ", calls
          endif
          if (amount.ne.1) then
             toterrs = toterrs + 1
             print *, "Sender amount is ", amount
          endif
       else if (wrank .eq. 1) then
          if (rcalls.ne.1) then
             toterrs = toterrs + 1
             print *, "Receiver calls is ", rcalls
          endif
          if (ramount.ne.1) then
             toterrs = toterrs + 1
             print *, "Receiver amount is ", ramount
          endif
       endif

       call mpi_allreduce( MPI_IN_PLACE, toterrs, 1, MPI_INT, MPI_SUM, &
      &      MPI_COMM_WORLD, ierr )
       if (wrank .eq. 0) then
          if (toterrs .eq. 0) then
             print *, " No Errors"
          else
             print *, " Found ", toterrs, " errors"
          endif
       endif
!
       call mpi_finalize( ierr )
       end
!
       subroutine mpi_send_f08ts( smsg, count, dtype, dest, tag, comm, ierr )
       use :: mpi_f08, my_noname => mpi_send_f08ts
       type(*), dimension(..), intent(in) :: smsg
       integer, intent(in) :: count, dest, tag
       type(MPI_Datatype), intent(in) :: dtype
       type(MPI_Comm), intent(in) :: comm
       integer, optional, intent(out) :: ierr

       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
!
       calls = calls + 1
       amount = amount + count
       if (present(ierr)) then
         call pmpi_send(smsg, count, dtype, dest, tag, comm, ierr)
       else
         call pmpi_send(smsg, count, dtype, dest, tag, comm)
       end if
       end
!
      subroutine mpi_recv_f08ts( rmsg, count, dtype, src, tag, comm, status, ierr )
       use :: mpi_f08, my_noname => mpi_recv_f08ts
       type(*), dimension(..), intent(in) :: rmsg
       integer, intent(in) :: count, src, tag
       type(MPI_Datatype), intent(in) :: dtype
       type(MPI_Comm), intent(in) :: comm
       type(MPI_Status) :: status
       integer, optional, intent(out) :: ierr

       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       rcalls = rcalls + 1
       ramount = ramount + 1
       if (present(ierr)) then
         call pmpi_recv(rmsg, count, dtype, src, tag, comm, status, ierr)
       else
         call pmpi_recv(rmsg, count, dtype, src, tag, comm, status)
       end if
       end
!
       subroutine init_counts()
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       calls = 0
       amount = 0
       rcalls = 0
       ramount = 0
       end
!
       subroutine mpi_pcontrol( ierr )
       integer ierr
       return
       end

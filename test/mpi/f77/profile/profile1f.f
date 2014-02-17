C -*- Mode: Fortran; -*-
C
C  (C) 2013 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
       program main
       include "mpif.h"
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
          call mpi_recv( rmsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,
     $         MPI_STATUS_IGNORE, ierr ) 
          if (rmsg(1) .ne. 3) then
             toterrs = toterrs + 1
             print *, "Unexpected received value ", rmsg(1)
          endif
       endif
C
C     check that we used the profiling versions of the routines
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

       call mpi_allreduce( MPI_IN_PLACE, toterrs, 1, MPI_INT, MPI_SUM,
     $      MPI_COMM_WORLD, ierr )
       if (wrank .eq. 0) then
          if (toterrs .eq. 0) then
             print *, " No Errors"
          else
             print *, " Found ", toterrs, " errors"
          endif
       endif
C
       call mpi_finalize( ierr )
       end
C
       subroutine mpi_send( smsg, count, dtype, dest, tag, comm, ierr )
       include "mpif.h"
       integer count, dtype, dest, tag, comm, ierr
       integer smsg(count)
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
c
       calls = calls + 1
       amount = amount + count
       call pmpi_send( smsg, count, dtype, dest, tag, comm, ierr )
       return
       end
C
      subroutine mpi_recv( rmsg, count, dtype, src, tag, comm, status,
     $     ierr ) 
       include "mpif.h"
       integer count, dtype, src, tag, comm, status(MPI_STATUS_SIZE),
     $      ierr 
       integer rmsg(count)
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       rcalls = rcalls + 1
       ramount = ramount + 1
       call pmpi_recv( rmsg, count, dtype, src, tag, comm, status, ierr
     $      ) 
       return
       end
C
       subroutine init_counts()
       common /myinfo/ calls, amount, rcalls, ramount
       integer calls, amount, rcalls, ramount
       calls = 0
       amount = 0
       rcalls = 0
       ramount = 0
       end
C
       subroutine mpi_pcontrol( ierr )
       integer ierr
       return
       end

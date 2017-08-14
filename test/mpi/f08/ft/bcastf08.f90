! -*- Mode: Fortran; -*- 

!
!  This test attempts collective bcast communication after a process in
!  the communicator has failed.
!

      program main
      use mpi_f08
      implicit none
      integer rank, size, rc, errclass, toterrs, errs, ierr
      integer deadprocs(1)
      character(len=100000) buf
      type(MPI_Group) world, newgroup
      type(MPI_Comm) newcomm

      deadprocs(1) = 1
      errs = 0

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN, &
     &                             ierr)

      call MPI_Comm_group(MPI_COMM_WORLD, world, ierr)
      call MPI_Group_excl(world, 1, deadprocs, newgroup, ierr);
      call MPI_Comm_create_group(MPI_COMM_WORLD,newgroup,0,newcomm,ierr)

      if (size .lt. 3) then
         write(*,*) "Must run with at least 3 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) buf = "No Errors"

!     do a small bcast first
      call MPI_Bcast(buf, 10, MPI_CHAR, 0, MPI_COMM_WORLD, rc)

      call MPI_Error_class(rc, errclass, ierr)
      if ((rc .ne. 0) .and. (errclass .ne. MPIX_ERR_PROC_FAILED)) then
         write(*,*) "Wrong error code (", errclass, &
     &              ") returned. Expected MPIX_ERR_PROC_FAILED"
         errs = errs+1
      end if

!     reset the non-root buffers
      if (rank .ne. 0) buf = ''

!     do a larger bcast
      call MPI_Bcast(buf, 100000, MPI_CHAR, 0, MPI_COMM_WORLD, rc)

      call MPI_Error_class(rc, errclass, ierr)
      if ((rc .ne. 0) .and. (errclass .ne. MPIX_ERR_PROC_FAILED)) then
         write(*,*) "Wrong error code (", errclass, &
     &              ") returned. Expected MPIX_ERR_PROC_FAILED"
         errs = errs+1
      end if

      call MPI_Reduce(errs,toterrs,1,MPI_INTEGER,MPI_SUM,0,newcomm,rc)
      if (rc .ne. 0) print *,"Failed to get errors from other processes"

      if (rank .eq. 0) then
         if (toterrs .ne. 0) then
            write(*,*) " Found ", toterrs, " errors"
         else
            write(*,*) " No Errors"
         end if
      end if

      call MPI_Group_free(world, ierr)
      call MPI_Group_free(newgroup, ierr)
      call MPI_Comm_free(newcomm, ierr)
      call MPI_Finalize(ierr)

      end program

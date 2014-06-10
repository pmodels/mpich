! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Test various combinations of periodic and non-periodic Cartesian
! communicators
!
      program main
      use mpi_f08
      integer errs, ierr
      integer ndims, nperiods, i, size
      integer source, dest
      type(mpi_comm) comm, newcomm
      integer maxdims
      parameter (maxdims=7)
      logical periods(maxdims), outperiods(maxdims)
      integer dims(maxdims), outdims(maxdims)
      integer outcoords(maxdims)

      errs = 0
      call mtest_init( ierr )

!
!     For upto 6 dimensions, test with periodicity in 0 through all
!     dimensions.  The test is computed by both:
!         get info about the created communicator
!         apply cart shift
!     Note that a dimension can have size one, so that these tests
!     can work with small numbers (even 1) of processes
!
      comm = MPI_COMM_WORLD
      call mpi_comm_size( comm, size, ierr )
      do ndims = 1, 6
         do nperiods = 0, ndims
            do i=1,ndims
               periods(i) = .false.
               dims(i)    = 0
            enddo
            do i=1,nperiods
               periods(i) = .true.
            enddo

            call mpi_dims_create( size, ndims, dims, ierr )
            call mpi_cart_create( comm, ndims, dims, periods, .false., &
      &           newcomm, ierr )

            if (newcomm .ne. MPI_COMM_NULL) then
               call mpi_cart_get( newcomm, maxdims, outdims, outperiods, &
      &              outcoords, ierr )
!               print *, 'Coords = '
               do i=1, ndims
!                  print *, i, '(', outcoords(i), ')'
                  if (periods(i) .neqv. outperiods(i)) then
                     errs = errs + 1
                     print *, ' Wrong value for periods ', i
                     print *, ' ndims = ', ndims
                  endif
               enddo

               do i=1, ndims
                  call mpi_cart_shift( newcomm, i-1, 1, source, dest, &
      &                 ierr )
                  if (outcoords(i) .eq. outdims(i)-1) then
                     if (periods(i)) then
                        if (dest .eq. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected rank, got proc_null'
                        endif
                     else
                        if (dest .ne. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected procnull, got ', dest
                        endif
                     endif
                  endif

                  call mpi_cart_shift( newcomm, i-1, -1, source, dest, &
      &                 ierr )
                  if (outcoords(i) .eq. 0) then
                     if (periods(i)) then
                        if (dest .eq. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected rank, got proc_null'
                        endif
                     else
                        if (dest .ne. MPI_PROC_NULL) then
                           errs = errs + 1
                           print *, 'Expected procnull, got ', dest
                        endif
                     endif
                  endif
               enddo
               call mpi_comm_free( newcomm, ierr )
            endif

         enddo
      enddo

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end

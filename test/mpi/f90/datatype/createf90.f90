!  
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi
        integer ierr
        integer errs
        integer nints, nadds, ndtypes, combiner
        integer nparms(2), dummy(1)
        integer (kind=MPI_ADDRESS_KIND) adummy(1)
        integer ntype1, nsize, ntype2, ntype3, i
!
!       Test the Type_create_f90_xxx routines
!
        errs = 0
        call mtest_init( ierr )

! integers with upto 9 are 4 bytes integers; r of 4 are 2 byte,
! and r of 2 is 1 byte
        call mpi_type_create_f90_integer( 9, ntype1, ierr )
!
!       Check with get contents and envelope...
        call mpi_type_get_envelope( ntype1, nints, nadds, ndtypes, &
                                    combiner, ierr )
        if (nadds .ne. 0) then
           errs = errs + 1
           print *, "There should be no addresses on created type (r=9)"
        endif
        if (ndtypes .ne. 0) then
           errs = errs + 1
           print *, "There should be no datatypes on created type (r=9)"
        endif
        if (nints .ne. 1) then
           errs = errs + 1
           print *, "There should be exactly 1 integer on create type (r=9)"
        endif
        if (combiner .ne. MPI_COMBINER_F90_INTEGER) then
           errs = errs + 1
           print *, "The combiner should be INTEGER, not ", combiner
        endif
        if (nints .eq. 1) then
           call mpi_type_get_contents( ntype1, 1, 0, 0, &
                                       nparms, adummy, dummy, ierr )
           if (nparms(1) .ne. 9) then
              errs = errs + 1
              print *, "parameter was ", nparms(1), " should be 9"
           endif
        endif
           
        call mpi_type_create_f90_integer( 8, ntype2, ierr )
        if (ntype1 .eq. ntype2) then
           errs = errs + 1
           print *, "Types with r = 8 and r = 9 are the same, ", &
                "should be distinct"
        endif

!
! Check that we don't create new types each time.  This test will fail only
! if the MPI implementation checks for un-freed types or runs out of space
        do i=1, 100000
           call mpi_type_create_f90_integer( 8, ntype3, ierr )
        enddo

        call mtest_finalize( errs )
        call mpi_finalize( ierr )
        
        end

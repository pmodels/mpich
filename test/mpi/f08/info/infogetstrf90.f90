!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

      program main
      use mpi_f08
      integer ierr, errs
      type(MPI_Info) inf
      integer nkeys, i, j, bufl, vall
      logical f
      character*(MPI_MAX_INFO_KEY) keys(3)
      character*(MPI_MAX_INFO_VAL) vals(3)
      integer valls(3)
      character*(MPI_MAX_INFO_VAL) val
      character*(MPI_MAX_INFO_VAL) canary

      data keys /"file", "sort", "host"/
! len_trim(val(i)) = i * 3
      data vals /"123", "123456", "123456789"/

      canary = "Canary"
      errs = 0

      call mtest_init( ierr )

! Create inf objects and set keys
      call mpi_info_create( inf, ierr )
      call mpi_info_set( inf, keys(1), vals(1), ierr )
      call mpi_info_set( inf, keys(2), vals(2), ierr )
      call mpi_info_set( inf, keys(3), vals(3), ierr )

! Check the number of keys
      call mpi_info_get_nkeys( inf, nkeys, ierr )
      if ( nkeys .ne. 3 ) then
         print *, ' Number of keys should be 3, is ', nkeys
      endif

! Case 1: invalid key
      bufl = 10
      val = canary
      call mpi_info_get_string( inf, "invkey", bufl, val, f, ierr )
      if ( f ) then
         errs = errs + 1
         print *, ' get_string succeeded for invalid key'
      endif
      if ( val .ne. canary ) then
         errs = errs + 1
         print *, ' get_string with invalid key changed val'
      endif

! Case 2: bufl = 0
      do i = 1, nkeys
         bufl = 0
         val = canary
         call mpi_info_get_string( inf, keys(i), bufl, val, f, ierr )
         if ( .not. f ) then
            errs = errs + 1
            print *, ' get_string failed for valid key ', keys(i)
         endif
         vall = i * 3
         if (bufl .ne. vall) then
            errs = errs + 1
            print *, ' get_string returned ', bufl, ' but must be ', vall
         endif
         if ( val .ne. canary ) then
            errs = errs + 1
            print *, ' get_string changes val while bufl is 0'
         endif
      enddo

! Case 3: bufl = 2 (very small number)
      do i = 1, nkeys
         bufl = 2
         val = canary
         call mpi_info_get_string( inf, keys(i), bufl, val, f, ierr )
         if ( .not. f ) then
            errs = errs + 1
            print *, ' get_string failed for valid key ', keys(i)
         endif
         vall = i * 3
         if (bufl .ne. vall) then
            errs = errs + 1
            print *, ' get_string returned ', bufl, ' but must be ', vall
         endif
         if ( val .ne. vals(i)(1:2) ) then
            errs = errs + 1
            print *, ' get_string returned incorrect val for key ', keys(i)
         endif
         do j = 3, MPI_MAX_INFO_VAL
            if ( val(j:j) .ne. " " ) then
                errs = errs + 1
                print *, "Found non blank in val at ", j
            endif
         enddo
      enddo

! Case 4: bufl = MPI_MAX_INFO_VAL (very large number)
      do i = 1, nkeys
         bufl = MPI_MAX_INFO_VAL
         call mpi_info_get_string( inf, keys(i), bufl, val, f, ierr )
         if ( .not. f ) then
            errs = errs + 1
            print *, ' get_string failed for valid key ', keys(i)
         endif
         vall = i * 3
         if (bufl .ne. vall ) then
            errs = errs + 1
            print *, ' get_string returned ', bufl, ' but must be ', vall
         endif
         if ( val .ne. vals(i) ) then
            errs = errs + 1
            print *, ' get_string returned incorrect val for key ', keys(i)
         endif
      enddo

      call mpi_info_free( inf, ierr )

      call mtest_finalize( errs )

      end

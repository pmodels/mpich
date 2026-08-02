C
C Copyright (C) by Argonne National Laboratory
C     See COPYRIGHT in top-level directory
C

C
C Test MPI_REDUCE with various builtin types
C
C This test catches potential internal datatype mapping issues.
C

      program main
      implicit none
      include 'mpif.h'

      integer rank, comm, size
      integer errs, ierr
      integer val_i, sum_i, ans_i
      real val_f, sum_f, ans_f
      double precision val_d, sum_d, ans_d

      errs = 0

      call mtest_init(ierr)
      comm = MPI_COMM_WORLD
      call MPI_Comm_rank(comm, rank, ierr)
      call MPI_Comm_size(comm, size, ierr)

C ---- INTEGER ----
      val_i = real(rank + 1)

      call MPI_REDUCE(val_i, sum_i, 1, MPI_INTEGER, MPI_SUM, 0, comm,
     & ierr)

      if (rank .eq. 0) then
         ans_i = size * (size + 1) / 2
         if (sum_i .ne. ans_i) then
            errs = errs + 1
            print *, 'MPI_Reduce on INTEGER: unexpected value'
            print *, '    Got ', sum_i, ', Expect ', ans_i
         endif
      endif

C ---- REAL ----
      val_f = real(rank + 1)

      call MPI_REDUCE(val_f, sum_f, 1, MPI_REAL, MPI_SUM,
     &     0, comm, ierr)

      if (rank .eq. 0) then
         ans_f = real(size * (size + 1) / 2)
         if (abs(sum_f - ans_f) .gt. 1.0e-4) then
            errs = errs + 1
            print *, 'MPI_Reduce on REAL: unexpected value'
            print *, '    Got ', sum_f, ', Expect ', ans_f
         endif
      endif

C ---- DOUBLE PRECISION ----
      val_d = dble(rank + 1)

      call MPI_REDUCE(val_d, sum_d, 1, MPI_DOUBLE_PRECISION, MPI_SUM,
     & 0, comm, ierr)

      if (rank .eq. 0) then
         ans_d = dble(size * (size + 1) / 2)
         if (abs(sum_d - ans_d) .gt. 1.0e-4) then
            errs = errs + 1
            print *, 'MPI_Reduce on DOUBLE PREC: '
     &           //'unexpected value'
            print *, '    Got ', sum_d, ', Expect ', ans_d
         endif
      endif

      call mtest_finalize(errs)
      end

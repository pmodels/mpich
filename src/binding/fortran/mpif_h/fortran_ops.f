!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

      subroutine real16_sum(A, B, N)
        real(16) A(N), B(N)
        integer N
        integer i

        DO i = 1, N
            B(i) = B(i) + A(i)
        END DO

        return
      end

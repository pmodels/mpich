!--> MPIR_Wtick

function MPI_Wtick_f08( ) Result(tick)
     use,intrinsic :: iso_c_binding, only: c_double
      use :: mpi_c_interface_nobuf, only: MPIR_Wtick_c
      DOUBLE PRECISION :: tick
      real(c_double)   :: tick_c

      tick_c = MPIR_Wtick_c()
      tick = tick_c
end function MPI_Wtick_f08
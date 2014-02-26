module mpi_c_interface_cdesc
implicit none

interface

function MPIR_Bsend_init_cdesc (buf, count, datatype, dest, tag, comm, request) &
            BIND(C, name="MPIR_Bsend_init_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype, C_Comm, C_request
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..)             :: buf
    integer(c_int),      value,intent(in)  :: count
    integer(c_Datatype), value,intent(in)  :: datatype
    integer(c_int),      value,intent(in)  :: dest
    integer(c_int),      value,intent(in)  :: tag
    integer(c_Comm),     value,intent(in)  :: comm
    integer(C_request),       intent(out)  :: request
    integer(c_int)                         :: res
end function MPIR_Bsend_init_cdesc

function MPIR_Buffer_attach_cdesc (buffer, size) &
            BIND(C, name="MPIR_Buffer_attach_cdesc") RESULT (res)
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..),asynchronous :: buffer
    integer(c_int),       value, intent(in) :: size
    integer(c_int)                          :: res
end function MPIR_Buffer_attach_cdesc

function MPIR_Bsend_cdesc (buf, count, datatype, dest, tag, comm) &
            BIND(C, name="MPIR_Bsend_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype, C_Comm
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..)             :: buf
    integer(c_int),      value,intent(in)  :: count
    integer(c_Datatype), value,intent(in)  :: datatype
    integer(c_int),      value,intent(in)  :: dest
    integer(c_int),      value,intent(in)  :: tag
    integer(c_Comm),     value,intent(in)  :: comm
    integer(c_int)                         :: res
end function MPIR_Bsend_cdesc

function MPIR_Irecv_cdesc (buf, count, datatype, source, tag, comm, request) &
            BIND(C, name="MPIR_Irecv_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype, C_Comm, C_Request
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..)             :: buf
    integer(c_int),      value,intent(in)  :: count
    integer(c_Datatype), value,intent(in)  :: datatype
    integer(c_int),      value,intent(in)  :: source
    integer(c_int),      value,intent(in)  :: tag
    integer(c_Comm),     value,intent(in)  :: comm
    integer(c_Request),        intent(out) :: request
    integer(c_int)                    :: res
end function MPIR_Irecv_cdesc


function MPIR_Isend_cdesc (buf, count, datatype, dest, tag, comm, request) &
            BIND(C, name="MPIR_Isend_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype, C_Comm, C_Request
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..)                 :: buf
    integer(c_int),      value,intent(in) :: count
    integer(C_Datatype), value,intent(in) :: datatype
    integer(c_int),      value,intent(in) :: dest
    integer(c_int),      value,intent(in) :: tag
    integer(C_Comm),     value,intent(in) :: comm
    integer(C_Request),       intent(out) :: request
    integer(c_int)                        :: res
end function MPIR_Isend_cdesc


function MPIR_Recv_cdesc (buf,count,datatype,source,tag,comm, status) &
           BIND(C, name="MPIR_Recv_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: c_Status, C_Datatype, C_Comm
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..)                 :: buf
    integer(c_int),      value,intent(in) :: count
    integer(C_Datatype), value,intent(in) :: datatype
    integer(c_int),      value,intent(in) :: source
    integer(c_int),      value,intent(in) :: tag
    integer(C_Comm),     value,intent(in) :: comm
    type(c_Status),       intent(out) :: status
    integer(c_int)                        :: res
end function MPIR_Recv_cdesc

function MPIR_Send_cdesc (buf,count,datatype,dest,tag,comm) &
           BIND(C, name="MPIR_Send_cdesc") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype, C_Comm
    use,intrinsic :: iso_c_binding, only: c_int
    type(*),dimension(..),    intent(in) :: buf
    integer(c_int),     value,intent(in) :: count
    integer(C_Datatype),value,intent(in) :: datatype
    integer(c_int),     value,intent(in) :: dest
    integer(c_int),     value,intent(in) :: tag
    integer(C_Comm),    value,intent(in) :: comm
    integer(c_int)                       :: res
end function MPIR_Send_cdesc

end interface

end module mpi_c_interface_cdesc

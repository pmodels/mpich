## MPIR_Request Objects

The request object is the most critical object in the MPICH communication
path. All our MPID interface are non-blocking, thus they all follow the
pattern of --
1. creating request objects
1. setup `cc_ptr`, ref_count, store necessary information
1. issue operation using the request as context
1. [progress]
1. complete or update the request as progress are made and relevant callbacks
or event function gets called
1. MPI_Wait or MPI_Test return success and reaps the request object

The request struct need hold necessary fields for all kinds of communications,
and all layers need touch the request object throughout its lifetime. Thus,
the request struct consists fields complexed by both the communication types
and code layers. It is by far the most complex part of MPICH code -- thus the
need for this separate note.

The MPIR_Request struct is defined in
[mpir_request.h](../../../src/include/mpir_request.h#L168-L225). Other than the
common fields, we have a big union to support different kinds of request.
However, like an iceberg, more complex fields are hidden by the device layer
macro `MPID_DEV_REQUEST_DECL`. We use the macro to add device-layer fields
so it can pull in the right device fields based on configuration.

In ch4, the device fields is a struct (`dev`) with a big union (`ch4`) with
another layer of union (`netmod` and `shm`), each wrapped with another macro
that is up to `configure` definitions. Each parts of the struct or union can
have big or nested definitions. They are accessed all over the code with all
similar long chain of names due to the super-nested nature, and often are
referred with additional macros. I am writing these notes out of certain
frustration -- it is easy for a developer to lose hours simply to trace what
is what, again and again.

Here are some pointers:
* Top-level: [mpir_request.h](../../../src/include/mpir_request.h#L168-L225)
* Ch4-level: [mpidpre.h](../../../src/mpid/ch4/include/mpidpre.h#L272-L304)<br/>
  Defines `MPIDI_REQUEST(req, field) -> req->dev.field` <br/>
  The most complex part of the union are `MPIDIG_req_t` and
  `MPIDI_OFI_request_t` (inside `netmod` union).
* AM: [mpidpre.h](../../../src/mpid/ch4/include/mpidpre.h#L207-L222)<br/>
  `MPIDIG_req_t` not only nests additional device layer (`netmod_am`,
  `shm_am`), it also allocates additional buffer `MPIDIG_req_ext_t`, which is
  a complex struct/union with definitions cover all AM protocols.
* OFI: [ofi_pre.h](../../../src/mpid/ch4/netmod/ofi/ofi_pre.h#L170-L188)<br/>
  The fixed fields `context` and `event_id` allows passing into libfabric API
  as context.
* UCX and POSIX are simple. UCX doesn't pass part of the request as context,
  in stead, ucx always generates and returns its own *context* (referred as
  `ucp_request`), and we simply store the `MPIR_Request` inside the
  `ucp_request`. POSIX doesn't need additional fields since it always uses AM.

The active message part contributes most of the complexity. We put all fields
used in different protocols together away from where they are being used
(callbacks, which are again mixed together). TODO: properly group data fields
and code by protocols/usage and make them as opaque to outside as possible.

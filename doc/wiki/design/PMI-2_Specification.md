# PMI-2 Specification

For the formal specification of PMI-2.

See [PMI v2 API](PMI_v2_API.md) for some discussions about
possible designs and some issues.

## Design Requirements

(WDG - I find it valuable to list objectives and requirements first.
Here's an initial list. They can and should be expanded, and the
consequences of each understood)

  - Scalable - Semantics of operations must permit scalable
    implementation
  - Efficient - Must provide MPI implementation with the information
    that it needs without requiring potentially expensive steps.
  - Complete - Must support all of MPI, including dynamic processes
  - Robust - Must handle failures and aborts, including any resources
    acquired by the MPI application.
  - Correct - Must avoid race conditions in the design
  - Portable - Must not assume a particular environment such as POSIX

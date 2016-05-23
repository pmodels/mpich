/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : MEMORY
      description : affects memory allocation and usage, including MPI object handles

cvars:
    - name        : MPIR_CVAR_ABORT_ON_LEAKED_HANDLES
      category    : MEMORY
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If true, MPI will call MPI_Abort at MPI_Finalize if any MPI object
        handles have been leaked.  For example, if MPI_Comm_dup is called
        without calling a corresponding MPI_Comm_free.  For uninteresting
        reasons, enabling this option may prevent all known object leaks from
        being reported.  MPICH must have been configure with
        "--enable-g=handlealloc" or better in order for this functionality to
        work.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include <stdio.h>

/* returns the name of the handle kind for debugging/logging purposes */
const char *MPIR_Handle_get_kind_str(int kind)
{
#define mpiu_name_case_(name_) case MPIR_##name_: return (#name_)
    switch (kind) {
        mpiu_name_case_(COMM);
        mpiu_name_case_(GROUP);
        mpiu_name_case_(DATATYPE);
        mpiu_name_case_(FILE);
        mpiu_name_case_(ERRHANDLER);
        mpiu_name_case_(OP);
        mpiu_name_case_(INFO);
        mpiu_name_case_(WIN);
        mpiu_name_case_(KEYVAL);
        mpiu_name_case_(ATTR);
        mpiu_name_case_(REQUEST);
        mpiu_name_case_(PROCGROUP);
        mpiu_name_case_(VCONN);
        mpiu_name_case_(GREQ_CLASS);
        default:
            return "unknown";
    }
#undef mpiu_name_case_
}

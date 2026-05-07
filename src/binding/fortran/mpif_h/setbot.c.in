/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"


#ifdef F77_NAME_UPPER
#define mpirinitc_ MPIRINITC
#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE)
/* leave name alone */
#else
#define mpirinitc_ mpirinitc
#endif
/* These functions are called from Fortran so only need prototypes in
   this file.  Note that the second-to-last argument is a character array, so
   we need to include the elements of the Fortran character "dope vector".
*/
FORT_DLL_SPEC void FORT_CALL mpirinitc_(void *si, void *ssi,
                                        void *bt, void *ip, void *c_ba,
                                        void *uw, void *ecsi,
                                        void *an FORT_MIXED_LEN(d1),
                                        void *asn FORT_MIXED_LEN(d1),
                                        void *we FORT_END_LEN(d1) FORT_END_LEN(d2));

/*
    # MPI-2, section 4.12.5, on the declaration of MPI_F_STATUS_IGNORE
    # MPI_F_STATUSES_IGNORE as global variables in mpi.h (!)
*/
int MPIR_F_NeedInit = 1;
void *MPIR_F_MPI_BOTTOM = 0;
void *MPIR_F_MPI_IN_PLACE = 0;
void *MPIR_F_MPI_BUFFER_AUTOMATIC = 0;
void *MPIR_F_MPI_UNWEIGHTED = 0;
/* MPI_F_STATUS_IGNORE etc must be declared within mpi.h (MPI-2 standard
   requires this) */
/*
void *MPI_F_STATUS_IGNORE   = 0;
void *MPI_F_STATUSES_IGNORE = 0;
*/
MPI_Fint *MPI_F_ERRCODES_IGNORE = 0;
void *MPI_F_ARGV_NULL = 0;
void *MPI_F_ARGVS_NULL = 0;
void *MPIR_F_MPI_WEIGHTS_EMPTY = 0;

FORT_DLL_SPEC void FORT_CALL mpirinitc_(void *si, void *ssi,
                                        void *bt, void *ip, void *c_ba,
                                        void *uw, void *ecsi,
                                        void *an FORT_MIXED_LEN(d1),
                                        void *asn FORT_MIXED_LEN(d1),
                                        void *we FORT_END_LEN(d1) FORT_END_LEN(d2))
{
    MPI_F_STATUS_IGNORE = (MPI_Fint *) si;
    MPI_F_STATUSES_IGNORE = (MPI_Fint *) ssi;
    MPIR_F_MPI_BOTTOM = bt;
    MPIR_F_MPI_IN_PLACE = ip;
    MPIR_F_MPI_BUFFER_AUTOMATIC = c_ba;
    MPIR_F_MPI_UNWEIGHTED = uw;
    MPI_F_ERRCODES_IGNORE = (MPI_Fint *) ecsi;
    MPI_F_ARGV_NULL = an;
    MPI_F_ARGVS_NULL = asn;
    MPIR_F_MPI_WEIGHTS_EMPTY = we;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi_fortimpl.h"
#include <assert.h>


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

static void info_set_int(MPI_Info info, const char *key, int val)
{
    char str[20];
    snprintf(str, 20, "%d", val);
    MPI_Info_set(info, key, str);
}

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

    MPIX_Init_fortran();

    MPIR_F_NeedInit = 0;
}

void MPIX_Init_fortran(void)
{
    int mpi_errno;
    MPI_Info info;
    mpi_errno = MPI_Abi_get_fortran_info(&info);
    assert(mpi_errno == MPI_SUCCESS);

    if (info == MPI_INFO_NULL) {
        /* MPI_Abi_set_fortran_info */
        MPI_Info_create(&info);
        info_set_int(info, "mpi_character_size", MPI_CHARACTER_SIZE);
        info_set_int(info, "mpi_logical_size", MPI_LOGICAL_SIZE);
        info_set_int(info, "mpi_integer_size", MPI_INTEGER_SIZE);
        info_set_int(info, "mpi_real_size", MPI_REAL_SIZE);
        info_set_int(info, "mpi_double_precision_size", MPI_DOUBLE_PRECISION_SIZE);
#ifdef MPI_LOGICAL1_SUPPORTED
        MPI_Info_set(info, "mpi_logical1_supported", "true");
#endif
#ifdef MPI_LOGICAL2_SUPPORTED
        MPI_Info_set(info, "mpi_logical2_supported", "true");
#endif
#ifdef MPI_LOGICAL4_SUPPORTED
        MPI_Info_set(info, "mpi_logical4_supported", "true");
#endif
#ifdef MPI_LOGICAL8_SUPPORTED
        MPI_Info_set(info, "mpi_logical8_supported", "true");
#endif
#ifdef MPI_LOGICAL16_SUPPORTED
        MPI_Info_set(info, "mpi_logical16_supported", "true");
#endif
#ifdef MPI_INTEGER1_SUPPORTED
        MPI_Info_set(info, "mpi_integer1_supported", "true");
#endif
#ifdef MPI_INTEGER2_SUPPORTED
        MPI_Info_set(info, "mpi_integer2_supported", "true");
#endif
#ifdef MPI_INTEGER4_SUPPORTED
        MPI_Info_set(info, "mpi_integer4_supported", "true");
#endif
#ifdef MPI_INTEGER8_SUPPORTED
        MPI_Info_set(info, "mpi_integer8_supported", "true");
#endif
#ifdef MPI_INTEGER16_SUPPORTED
        MPI_Info_set(info, "mpi_integer16_supported", "true");
#endif
#ifdef MPI_REAL2_SUPPORTED
        MPI_Info_set(info, "mpi_real2_supported", "true");
#endif
#ifdef MPI_REAL4_SUPPORTED
        MPI_Info_set(info, "mpi_real4_supported", "true");
#endif
#ifdef MPI_REAL8_SUPPORTED
        MPI_Info_set(info, "mpi_real8_supported", "true");
#endif
#ifdef MPI_REAL16_SUPPORTED
        MPI_Info_set(info, "mpi_real16_supported", "true");
#endif
#ifdef MPI_COMPLEX4_SUPPORTED
        MPI_Info_set(info, "mpi_complex4_supported", "true");
#endif
#ifdef MPI_COMPLEX8_SUPPORTED
        MPI_Info_set(info, "mpi_complex8_supported", "true");
#endif
#ifdef MPI_COMPLEX16_SUPPORTED
        MPI_Info_set(info, "mpi_complex16_supported", "true");
#endif
#ifdef MPI_COMPLEX32_SUPPORTED
        MPI_Info_set(info, "mpi_complex32_supported", "true");
#endif
#ifdef MPI_DOUBLE_PRECISION_SUPPORTED
        MPI_Info_set(info, "mpi_double_precision_supported", "true");
#endif
        mpi_errno = MPI_Abi_set_fortran_info(info);
        assert(mpi_errno == MPI_SUCCESS);

        MPI_Info_free(&info);

        /* MPI_Abi_set_fortran_boolean */
#if MPI_LOGICAL_SIZE == 1
        int8_t true_val = F77_TRUE_VALUE;
        int8_t false_val = F77_FALSE_VALUE;
#elif MPI_LOGICAL_SIZE == 2
        int16_t true_val = F77_TRUE_VALUE;
        int16_t false_val = F77_FALSE_VALUE;
#elif MPI_LOGICAL_SIZE == 4
        int32_t true_val = F77_TRUE_VALUE;
        int32_t false_val = F77_FALSE_VALUE;
#elif MPI_LOGICAL_SIZE == 8
        int64_t true_val = F77_TRUE_VALUE;
        int64_t false_val = F77_FALSE_VALUE;
#else
#error Configure did not define a good MPI_LOGICAL_SIZE!
#endif
        mpi_errno =
            MPI_Abi_set_fortran_booleans(MPI_LOGICAL_SIZE, (void *) &true_val, (void *) &false_val);
        assert(mpi_errno == MPI_SUCCESS);
    }
}

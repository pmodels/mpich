/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include <stdlib.h>
#include <limits.h>
#include "datatype.h"

/* MPI datatype debugging helper routines.
 *
 * The one you want to call is:
 *   MPIR_Datatype_debug(MPI_Datatype type, int array_ct)
 *
 * The "array_ct" value tells the call how many array values to print
 * for struct, indexed, and blockindexed types.
 *
 */

static void contents_printf(MPI_Datatype type, int depth, int array_ct);
static char *depth_spacing(int depth) ATTRIBUTE((unused));

#define NR_TYPE_CUTOFF 6        /* Number of types to display before truncating
                                 * output. 6 picked as arbitrary cutoff */

/* note: this isn't really "error handling" per se, but leave these comments
 * because Bill uses them for coverage analysis.
 */

/* --BEGIN ERROR HANDLING-- */

void MPII_Datatype_printf(MPI_Datatype type,
                          int depth, MPI_Aint displacement, int blocklength, int header)
{
#ifdef MPL_USE_DBG_LOGGING
    char *string;
    MPI_Aint size;
    MPI_Aint extent, true_lb, true_ub, lb, ub;

    if (HANDLE_IS_BUILTIN(type)) {
        string = MPIR_Datatype_builtin_to_string(type);
        MPIR_Assert(string != NULL);
    } else {
        MPIR_Datatype *type_ptr;

        MPIR_Datatype_get_ptr(type, type_ptr);
        MPIR_Assert(type_ptr != NULL);
        string = MPIR_Datatype_combiner_to_string(type_ptr->contents->combiner);
        MPIR_Assert(string != NULL);
    }

    MPIR_Datatype_get_size_macro(type, size);
    MPIR_Type_get_true_extent_impl(type, &true_lb, &extent);
    true_ub = extent + true_lb;
    MPIR_Type_get_extent_impl(type, &lb, &extent);
    ub = extent + lb;

    if (header == 1) {
        /*               012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 */
        MPL_DBG_OUT(MPIR_DBG_DATATYPE,
                    "------------------------------------------------------------------------------------------------------------------------------------------\n");
        MPL_DBG_OUT(MPIR_DBG_DATATYPE,
                    "depth                   type         size       extent      true_lb      true_ub           lb           ub         disp       blklen\n");
        MPL_DBG_OUT(MPIR_DBG_DATATYPE,
                    "------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,
                    (MPL_DBG_FDEST,
                     "%5d  %21s  %11d  " MPI_AINT_FMT_DEC_SPEC "  " MPI_AINT_FMT_DEC_SPEC "  "
                     MPI_AINT_FMT_DEC_SPEC "  " MPI_AINT_FMT_DEC_SPEC " " MPI_AINT_FMT_DEC_SPEC " "
                     MPI_AINT_FMT_DEC_SPEC "  %11d", depth, string, (int) size, (MPI_Aint) extent,
                     (MPI_Aint) true_lb, (MPI_Aint) true_ub, (MPI_Aint) lb,
                     (MPI_Aint) ub, (MPI_Aint) displacement, (int) blocklength));
#endif
    return;
}

/* --END ERROR HANDLING-- */

/* longest string is 21 characters */
char *MPIR_Datatype_builtin_to_string(MPI_Datatype type)
{
    static char t_char[] = "MPI_CHAR";
    static char t_uchar[] = "MPI_UNSIGNED_CHAR";
    static char t_byte[] = "MPI_BYTE";
    static char t_wchar_t[] = "MPI_WCHAR";
    static char t_short[] = "MPI_SHORT";
    static char t_ushort[] = "MPI_UNSIGNED_SHORT";
    static char t_int[] = "MPI_INT";
    static char t_uint[] = "MPI_UNSIGNED";
    static char t_long[] = "MPI_LONG";
    static char t_ulong[] = "MPI_UNSIGNED_LONG";
    static char t_float[] = "MPI_FLOAT";
    static char t_double[] = "MPI_DOUBLE";
    static char t_longdouble[] = "MPI_LONG_DOUBLE";
    static char t_longlongint[] = "MPI_LONG_LONG_INT";
    static char t_longlong[] = "MPI_LONG_LONG";
    static char t_ulonglong[] = "MPI_UNSIGNED_LONG_LONG";
    static char t_schar[] = "MPI_SIGNED_CHAR";

    static char t_packed[] = "MPI_PACKED";
    static char t_lb[] = "MPI_LB";
    static char t_ub[] = "MPI_UB";

    static char t_floatint[] = "MPI_FLOAT_INT";
    static char t_doubleint[] = "MPI_DOUBLE_INT";
    static char t_longint[] = "MPI_LONG_INT";
    static char t_shortint[] = "MPI_SHORT_INT";
    static char t_2int[] = "MPI_2INT";
    static char t_longdoubleint[] = "MPI_LONG_DOUBLE_INT";

    static char t_complex[] = "MPI_COMPLEX";
    static char t_doublecomplex[] = "MPI_DOUBLE_COMPLEX";
    static char t_logical[] = "MPI_LOGICAL";
    static char t_real[] = "MPI_REAL";
    static char t_doubleprecision[] = "MPI_DOUBLE_PRECISION";
    static char t_integer[] = "MPI_INTEGER";
    static char t_2integer[] = "MPI_2INTEGER";
    static char t_2real[] = "MPI_2REAL";
    static char t_2doubleprecision[] = "MPI_2DOUBLE_PRECISION";
    static char t_character[] = "MPI_CHARACTER";

    if (type == MPI_CHAR)
        return t_char;
    if (type == MPI_UNSIGNED_CHAR)
        return t_uchar;
    if (type == MPI_SIGNED_CHAR)
        return t_schar;
    if (type == MPI_BYTE)
        return t_byte;
    if (type == MPI_WCHAR)
        return t_wchar_t;
    if (type == MPI_SHORT)
        return t_short;
    if (type == MPI_UNSIGNED_SHORT)
        return t_ushort;
    if (type == MPI_INT)
        return t_int;
    if (type == MPI_UNSIGNED)
        return t_uint;
    if (type == MPI_LONG)
        return t_long;
    if (type == MPI_UNSIGNED_LONG)
        return t_ulong;
    if (type == MPI_FLOAT)
        return t_float;
    if (type == MPI_DOUBLE)
        return t_double;
    if (type == MPI_LONG_DOUBLE)
        return t_longdouble;
    if (type == MPI_LONG_LONG_INT)
        return t_longlongint;
    if (type == MPI_LONG_LONG)
        return t_longlong;
    if (type == MPI_UNSIGNED_LONG_LONG)
        return t_ulonglong;

    if (type == MPI_PACKED)
        return t_packed;
    if (type == MPI_LB)
        return t_lb;
    if (type == MPI_UB)
        return t_ub;

    if (type == MPI_FLOAT_INT)
        return t_floatint;
    if (type == MPI_DOUBLE_INT)
        return t_doubleint;
    if (type == MPI_LONG_INT)
        return t_longint;
    if (type == MPI_SHORT_INT)
        return t_shortint;
    if (type == MPI_2INT)
        return t_2int;
    if (type == MPI_LONG_DOUBLE_INT)
        return t_longdoubleint;

    if (type == MPI_COMPLEX)
        return t_complex;
    if (type == MPI_DOUBLE_COMPLEX)
        return t_doublecomplex;
    if (type == MPI_LOGICAL)
        return t_logical;
    if (type == MPI_REAL)
        return t_real;
    if (type == MPI_DOUBLE_PRECISION)
        return t_doubleprecision;
    if (type == MPI_INTEGER)
        return t_integer;
    if (type == MPI_2INTEGER)
        return t_2integer;
    if (type == MPI_2REAL)
        return t_2real;
    if (type == MPI_2DOUBLE_PRECISION)
        return t_2doubleprecision;
    if (type == MPI_CHARACTER)
        return t_character;

    return NULL;
}

/* MPIR_Datatype_combiner_to_string(combiner)
 *
 * Converts a numeric combiner into a pointer to a string used for printing.
 *
 * longest string is 16 characters.
 */
char *MPIR_Datatype_combiner_to_string(int combiner)
{
    static char c_named[] = "named";
    static char c_contig[] = "contig";
    static char c_vector[] = "vector";
    static char c_hvector[] = "hvector";
    static char c_indexed[] = "indexed";
    static char c_hindexed[] = "hindexed";
    static char c_struct[] = "struct";
    static char c_dup[] = "dup";
    static char c_hvector_integer[] = "hvector_integer";
    static char c_hindexed_integer[] = "hindexed_integer";
    static char c_indexed_block[] = "indexed_block";
    static char c_hindexed_block[] = "hindexed_block";
    static char c_struct_integer[] = "struct_integer";
    static char c_subarray[] = "subarray";
    static char c_darray[] = "darray";
    static char c_f90_real[] = "f90_real";
    static char c_f90_complex[] = "f90_complex";
    static char c_f90_integer[] = "f90_integer";
    static char c_resized[] = "resized";

    if (combiner == MPI_COMBINER_NAMED)
        return c_named;
    if (combiner == MPI_COMBINER_CONTIGUOUS)
        return c_contig;
    if (combiner == MPI_COMBINER_VECTOR)
        return c_vector;
    if (combiner == MPI_COMBINER_HVECTOR)
        return c_hvector;
    if (combiner == MPI_COMBINER_INDEXED)
        return c_indexed;
    if (combiner == MPI_COMBINER_HINDEXED)
        return c_hindexed;
    if (combiner == MPI_COMBINER_STRUCT)
        return c_struct;
    if (combiner == MPI_COMBINER_DUP)
        return c_dup;
    if (combiner == MPI_COMBINER_HVECTOR_INTEGER)
        return c_hvector_integer;
    if (combiner == MPI_COMBINER_HINDEXED_INTEGER)
        return c_hindexed_integer;
    if (combiner == MPI_COMBINER_INDEXED_BLOCK)
        return c_indexed_block;
    if (combiner == MPI_COMBINER_HINDEXED_BLOCK)
        return c_hindexed_block;
    if (combiner == MPI_COMBINER_STRUCT_INTEGER)
        return c_struct_integer;
    if (combiner == MPI_COMBINER_SUBARRAY)
        return c_subarray;
    if (combiner == MPI_COMBINER_DARRAY)
        return c_darray;
    if (combiner == MPI_COMBINER_F90_REAL)
        return c_f90_real;
    if (combiner == MPI_COMBINER_F90_COMPLEX)
        return c_f90_complex;
    if (combiner == MPI_COMBINER_F90_INTEGER)
        return c_f90_integer;
    if (combiner == MPI_COMBINER_RESIZED)
        return c_resized;

    return NULL;
}

/* --BEGIN DEBUG-- */
/*
 * You must configure MPICH2 with the logging option enabled (--enable-g=log)
 * for these routines to print - in which case, they use the same options
 * as the logging code, including print to file and control by class (DATATYPE)
 */
void MPIR_Datatype_debug(MPI_Datatype type, int array_ct)
{
#if (defined HAVE_ERROR_CHECKING) && (defined MPL_USE_DBG_LOGGING)
    const char *string;
#endif
    MPIR_Datatype *dtp ATTRIBUTE((unused));

    /* can get a NULL type a number of different ways, including not having
     * fortran support included.
     */
    if (type == MPI_DATATYPE_NULL) {
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,
                        (MPL_DBG_FDEST, "# MPIU_Datatype_debug: MPI_Datatype = MPI_DATATYPE_NULL"));
        return;
    }
#if (defined HAVE_ERROR_CHECKING) && (defined MPL_USE_DBG_LOGGING)
    if (HANDLE_IS_BUILTIN(type)) {
        string = MPIR_Datatype_builtin_to_string(type);
        MPIR_Assert(string != NULL);
    } else {
        string = "derived";
    }
    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                        "# MPIU_Datatype_debug: MPI_Datatype = 0x%0x (%s)", type,
                                        string));
#endif

    if (HANDLE_IS_BUILTIN(type))
        return;

    MPIR_Datatype_get_ptr(type, dtp);
    MPIR_Assert(dtp != NULL);

#if (defined HAVE_ERROR_CHECKING) && (defined MPL_USE_DBG_LOGGING)
    string = MPIR_Datatype_builtin_to_string(dtp->basic_type);
    MPIR_Assert(string != NULL);

    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                        "# Size = " MPI_AINT_FMT_DEC_SPEC ", Extent = "
                                        MPI_AINT_FMT_DEC_SPEC ", LB = " MPI_AINT_FMT_DEC_SPEC
                                        ", UB = " MPI_AINT_FMT_DEC_SPEC ", Extent = "
                                        MPI_AINT_FMT_DEC_SPEC ", Element Size = "
                                        MPI_AINT_FMT_DEC_SPEC " (%s), %s", (MPI_Aint) dtp->size,
                                        (MPI_Aint) dtp->extent, (MPI_Aint) dtp->lb,
                                        (MPI_Aint) dtp->ub, (MPI_Aint) dtp->extent,
                                        (MPI_Aint) dtp->builtin_element_size,
                                        dtp->builtin_element_size ==
                                        -1 ? "multiple types" :
                                        string,
                                        dtp->is_contig ? "is N contig" : "is not N contig"));
#endif

    MPL_DBG_OUT(MPIR_DBG_DATATYPE, "# Contents:");
    contents_printf(type, 0, array_ct);

    MPL_DBG_OUT(MPIR_DBG_DATATYPE, "# Typerep:");
    MPIR_Typerep_debug(type);
}

static char *depth_spacing(int depth)
{
    static char d0[] = "";
    static char d1[] = "  ";
    static char d2[] = "    ";
    static char d3[] = "      ";
    static char d4[] = "        ";
    static char d5[] = "          ";

    switch (depth) {
        case 0:
            return d0;
        case 1:
            return d1;
        case 2:
            return d2;
        case 3:
            return d3;
        case 4:
            return d4;
        default:
            return d5;
    }
}

static void contents_printf(MPI_Datatype type, int depth, int array_ct)
{
    int i;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    MPI_Aint *aints = NULL;
    MPI_Datatype *types = NULL;
    int *ints = NULL;
    MPI_Aint *counts = NULL;

#if (defined HAVE_ERROR_CHECKING) && (defined MPL_USE_DBG_LOGGING)
    if (HANDLE_IS_BUILTIN(type)) {
        const char *string = MPIR_Datatype_builtin_to_string(type);
        MPIR_Assert(string != NULL);
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %stype: %s\n",
                                            depth_spacing(depth), string));
        return;
    }
#endif

    MPIR_Datatype_get_ptr(type, dtp);
    cp = dtp->contents;

    if (cp == NULL) {
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# <NULL>\n"));
        return;
    }

    MPIR_Datatype_access_contents(cp, &ints, &aints, &counts, &types);

    /* FIXME: large count datatype need processed separately */
    MPIR_Assert(cp->nr_counts == 0);

#if (defined HAVE_ERROR_CHECKING) && (defined MPL_USE_DBG_LOGGING)
    {
        const char *string = MPIR_Datatype_combiner_to_string(cp->combiner);
        MPIR_Assert(string != NULL);
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %scombiner: %s",
                                            depth_spacing(depth), string));
    }
#endif

    switch (cp->combiner) {
        case MPI_COMBINER_NAMED:
        case MPI_COMBINER_DUP:
            return;
        case MPI_COMBINER_CONTIGUOUS:
            MPIR_Assert((ints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %scontig ct = %d\n",
                                                depth_spacing(depth), *ints));
            contents_printf(*types, depth + 1, array_ct);
            return;
        case MPI_COMBINER_VECTOR:
            MPIR_Assert((ints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "# %svector ct = %d, blk = %d, str = %d\n",
                                                depth_spacing(depth), ints[0], ints[1], ints[2]));
            contents_printf(*types, depth + 1, array_ct);
            return;
        case MPI_COMBINER_HVECTOR:
            MPIR_Assert((ints != NULL) && (aints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "# %shvector ct = %d, blk = %d, str = "
                                                MPI_AINT_FMT_DEC_SPEC "\n",
                                                depth_spacing(depth), ints[0],
                                                ints[1], (MPI_Aint) aints[0]));
            contents_printf(*types, depth + 1, array_ct);
            return;
        case MPI_COMBINER_INDEXED:
            MPIR_Assert((ints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %sindexed ct = %d:",
                                                depth_spacing(depth), ints[0]));
            for (i = 0; i < array_ct && i < ints[0]; i++) {
                MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                    "# %s  indexed [%d]: blk = %d, disp = %d\n",
                                                    depth_spacing(depth),
                                                    i,
                                                    ints[i + 1], ints[i + (cp->nr_ints / 2) + 1]));
                contents_printf(*types, depth + 1, array_ct);
            }
            return;
        case MPI_COMBINER_HINDEXED:
            MPIR_Assert((ints != NULL) && (aints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %shindexed ct = %d:",
                                                depth_spacing(depth), ints[0]));
            for (i = 0; i < array_ct && i < ints[0]; i++) {
                MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                    "# %s  hindexed [%d]: blk = %d, disp = "
                                                    MPI_AINT_FMT_DEC_SPEC "\n",
                                                    depth_spacing(depth), i,
                                                    (int) ints[i + 1], (MPI_Aint) aints[i]));
                contents_printf(*types, depth + 1, array_ct);
            }
            return;
        case MPI_COMBINER_STRUCT:
            MPIR_Assert((ints != NULL) && (aints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %sstruct ct = %d:",
                                                depth_spacing(depth), (int) ints[0]));
            for (i = 0; i < array_ct && i < ints[0]; i++) {
                MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                    "# %s  struct[%d]: blk = %d, disp = "
                                                    MPI_AINT_FMT_DEC_SPEC "\n",
                                                    depth_spacing(depth), i,
                                                    (int) ints[i + 1], (MPI_Aint) aints[i]));
                contents_printf(types[i], depth + 1, array_ct);
            }
            return;
        case MPI_COMBINER_SUBARRAY:
            MPIR_Assert((ints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %ssubarray ct = %d:",
                                                depth_spacing(depth), (int) ints[0]));
            for (i = 0; i < array_ct && i < ints[0]; i++) {
                MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                    "# %s  sizes[%d] = %d subsizes[%d] = %d starts[%d] = %d\n",
                                                    depth_spacing(depth),
                                                    i, (int) ints[i + 1],
                                                    i, (int) ints[i + ints[0] + 1],
                                                    i, (int) ints[2 * ints[0] + 1]));
            }
            contents_printf(*types, depth + 1, array_ct);
            return;

        case MPI_COMBINER_RESIZED:
            MPIR_Assert((aints != NULL) && (types != NULL));
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,
                            (MPL_DBG_FDEST,
                             "# %sresized lb = " MPI_AINT_FMT_DEC_SPEC " extent = "
                             MPI_AINT_FMT_DEC_SPEC "\n", depth_spacing(depth), aints[0], aints[1]));
            contents_printf(*types, depth + 1, array_ct);
            return;
        default:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "# %sunhandled combiner",
                                                depth_spacing(depth)));
            return;
    }
}

/* --END DEBUG-- */

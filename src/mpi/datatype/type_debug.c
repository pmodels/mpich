/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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


void MPII_Datatype_dot_printf(MPI_Datatype type, int depth, int header);
void MPII_Datatype_contents_printf(MPI_Datatype type, int depth, int acount);
void MPII_Dataloop_dot_printf(MPIR_Dataloop *loop_p, int depth, int header);
static char *MPII_Datatype_depth_spacing(int depth) ATTRIBUTE((unused));

#define NR_TYPE_CUTOFF 6 /* Number of types to display before truncating
			    output. 6 picked as arbitrary cutoff */

/* note: this isn't really "error handling" per se, but leave these comments
 * because Bill uses them for coverage analysis.
 */

/* --BEGIN ERROR HANDLING-- */
void MPII_Dataloop_dot_printf(MPIR_Dataloop *loop_p,
			       int depth,
			       int header)
{
    int i;

    if (loop_p == NULL) {
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"<null dataloop>\n"));
	return;
    }

    if (header) {
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		    /* graphviz does not like the 0xNNN format */
				   "digraph %lld {   {", (long long int)loop_p));
    }

    switch (loop_p->kind & DLOOP_KIND_MASK) {
	case DLOOP_KIND_CONTIG:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	    "      dl%d [shape = record, label = \"contig |{ ct = %d; el_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext = " MPI_AINT_FMT_DEC_SPEC " }\"];",
			    depth,
			    (int) loop_p->loop_params.c_t.count,
			    (MPI_Aint) loop_p->el_size,
			    (MPI_Aint) loop_p->el_extent));
	    break;
	case DLOOP_KIND_VECTOR:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	    "      dl%d [shape = record, label = \"vector |{ ct = %d; blk = %d; str = " MPI_AINT_FMT_DEC_SPEC "; el_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext =  "MPI_AINT_FMT_DEC_SPEC " }\"];",
			    depth,
			    (int) loop_p->loop_params.v_t.count,
			    (int) loop_p->loop_params.v_t.blocksize,
			    (MPI_Aint) loop_p->loop_params.v_t.stride,
			    (MPI_Aint) loop_p->el_size,
			    (MPI_Aint) loop_p->el_extent));
	    break;
	case DLOOP_KIND_INDEXED:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	    "      dl%d [shape = record, label = \"indexed |{ ct = %d; tot_blks = %d; regions = ",
			    depth,
			    (int) loop_p->loop_params.i_t.count,
			    (int) loop_p->loop_params.i_t.total_blocks));

	    for (i=0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.i_t.count; i++) {
		if (i + 1 < loop_p->loop_params.i_t.count) {
		    /* more regions after this one */
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		    "\\n(" MPI_AINT_FMT_DEC_SPEC ", %d), ",
			  (MPI_Aint) loop_p->loop_params.i_t.offset_array[i],
		          (int) loop_p->loop_params.i_t.blocksize_array[i]));
		}
		else {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		           "\\n(" MPI_AINT_FMT_DEC_SPEC ", %d); ",
		           (MPI_Aint) loop_p->loop_params.i_t.offset_array[i],
			   (int) loop_p->loop_params.i_t.blocksize_array[i]));
		}
	    }
	    if (i < loop_p->loop_params.i_t.count) {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"\\n...; ");
	    }

	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
				       "\\nel_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext = " MPI_AINT_FMT_DEC_SPEC " }\"];\n",
				       (MPI_Aint) loop_p->el_size,
				       (MPI_Aint) loop_p->el_extent));
	    break;
	case DLOOP_KIND_BLOCKINDEXED:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	    "      dl%d [shape = record, label = \"blockindexed |{ ct = %d; blk = %d; disps = ",
			    depth,
			    (int) loop_p->loop_params.bi_t.count,
			    (int) loop_p->loop_params.bi_t.blocksize));

	    for (i=0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.bi_t.count; i++) {
		if (i + 1 < loop_p->loop_params.bi_t.count) {
		    /* more regions after this one */
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		        MPI_AINT_FMT_DEC_SPEC ",\\n ",
			(MPI_Aint) loop_p->loop_params.bi_t.offset_array[i]));
		}
		else {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		         MPI_AINT_FMT_DEC_SPEC "; ",
		         (MPI_Aint) loop_p->loop_params.bi_t.offset_array[i]));
		}
	    }
	    if (i < loop_p->loop_params.bi_t.count) {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"...; ");
	    }

	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
				      "\\nel_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext = " MPI_AINT_FMT_DEC_SPEC " }\"];",
				       (MPI_Aint) loop_p->el_size,
				       (MPI_Aint) loop_p->el_extent));
	    break;
	case DLOOP_KIND_STRUCT:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	    "      dl%d [shape = record, label = \"struct | {ct = %d; blks = ",
			    depth,
			    (int) loop_p->loop_params.s_t.count));
	    for (i=0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.s_t.count; i++) {
		if (i + 1 < loop_p->loop_params.s_t.count) {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"%d, ",
			    (int) loop_p->loop_params.s_t.blocksize_array[i]));
		}
		else {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"%d; ",
			    (int) loop_p->loop_params.s_t.blocksize_array[i]));
		}
	    }
	    if (i < loop_p->loop_params.s_t.count) {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"...; disps = ");
	    }
	    else {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"disps = ");
	    }

	    for (i=0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.s_t.count; i++) {
		if (i + 1 < loop_p->loop_params.s_t.count) {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,MPI_AINT_FMT_DEC_SPEC ", ",
			    (MPI_Aint) loop_p->loop_params.s_t.offset_array[i]));
		}
		else {
		    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,MPI_AINT_FMT_DEC_SPEC "; ",
			    (MPI_Aint) loop_p->loop_params.s_t.offset_array[i]));
		}
	    }
	    if (i < loop_p->loop_params.s_t.count) {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"... }\"];");
	    }
	    else {
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"}\"];");
	    }
	    break;
	default:
	    MPIR_Assert(0);
    }

    if (!(loop_p->kind & DLOOP_FINAL_MASK)) {
	/* more loops to go; recurse */
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
				   "      dl%d -> dl%d;\n", depth, depth + 1));
	switch (loop_p->kind & DLOOP_KIND_MASK) {
	    case DLOOP_KIND_CONTIG:
		MPII_Dataloop_dot_printf(loop_p->loop_params.c_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_VECTOR:
		MPII_Dataloop_dot_printf(loop_p->loop_params.v_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_INDEXED:
		MPII_Dataloop_dot_printf(loop_p->loop_params.i_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_BLOCKINDEXED:
		MPII_Dataloop_dot_printf(loop_p->loop_params.bi_t.dataloop, depth + 1, 0);
		break;
	    case DLOOP_KIND_STRUCT:
		for (i=0; i < loop_p->loop_params.s_t.count; i++) {
		    MPII_Dataloop_dot_printf(loop_p->loop_params.s_t.dataloop_array[i],
					      depth + 1, 0);
		}
		break;
	    default:
		MPL_DBG_OUT(MPIR_DBG_DATATYPE,"      < unsupported type >");
	}
    }


    if (header) {
	MPL_DBG_OUT(MPIR_DBG_DATATYPE,"   }\n}");
    }
    return;
}

void MPII_Datatype_dot_printf(MPI_Datatype type,
                              int depth,
                              int header)
{
    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "MPII_Datatype_dot_printf: type is a basic");
        return;
    }
    else {
        MPIR_Datatype *dt_p;
        MPIR_Dataloop *loop_p;

        MPIR_Datatype_get_ptr(type, dt_p);
        loop_p = dt_p->dataloop;

        MPII_Dataloop_dot_printf(loop_p, depth, header);
        return;
    }
}

void MPII_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header)
{
#ifdef MPL_USE_DBG_LOGGING
    char *string;
    MPI_Aint size;
    MPI_Aint extent, true_lb, true_ub, lb, ub, sticky_lb, sticky_ub;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	string = MPIR_Datatype_builtin_to_string(type);
	if (type == MPI_LB) sticky_lb = 1;
	else sticky_lb = 0;
	if (type == MPI_UB) sticky_ub = 1;
	else sticky_ub = 0;
    }
    else {
	MPIR_Datatype *type_ptr;

	MPIR_Datatype_get_ptr(type, type_ptr);
	string = MPIR_Datatype_combiner_to_string(type_ptr->contents->combiner);
	sticky_lb = type_ptr->has_sticky_lb;
	sticky_ub = type_ptr->has_sticky_ub;
    }

    MPIR_Datatype_get_size_macro(type, size);
    MPIR_Type_get_true_extent_impl(type, &true_lb, &extent);
    true_ub = extent + true_lb;
    MPIR_Type_get_extent_impl(type, &lb, &extent);
    ub = extent + lb;

    if (header == 1) {
	/*               012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 */
	MPL_DBG_OUT(MPIR_DBG_DATATYPE,"------------------------------------------------------------------------------------------------------------------------------------------\n");
	MPL_DBG_OUT(MPIR_DBG_DATATYPE,"depth                   type         size       extent      true_lb      true_ub           lb(s)           ub(s)         disp       blklen\n");
	MPL_DBG_OUT(MPIR_DBG_DATATYPE,"------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"%5d  %21s  %11d  " MPI_AINT_FMT_DEC_SPEC "  " MPI_AINT_FMT_DEC_SPEC "  " MPI_AINT_FMT_DEC_SPEC "  " MPI_AINT_FMT_DEC_SPEC "(" MPI_AINT_FMT_DEC_SPEC ")  " MPI_AINT_FMT_DEC_SPEC "(" MPI_AINT_FMT_DEC_SPEC ")  " MPI_AINT_FMT_DEC_SPEC "  %11d",
		    depth,
		    string,
		    (int) size,
		    (MPI_Aint) extent,
		    (MPI_Aint) true_lb,
		    (MPI_Aint) true_ub,
		    (MPI_Aint) lb,
		    (MPI_Aint) sticky_lb,
		    (MPI_Aint) ub,
		    (MPI_Aint) sticky_ub,
		    (MPI_Aint) displacement,
		    (int) blocklength));
#endif
    return;
}
/* --END ERROR HANDLING-- */

/* longest string is 21 characters */
char *MPIR_Datatype_builtin_to_string(MPI_Datatype type)
{
    static char t_char[]             = "MPI_CHAR";
    static char t_uchar[]            = "MPI_UNSIGNED_CHAR";
    static char t_byte[]             = "MPI_BYTE";
    static char t_wchar_t[]          = "MPI_WCHAR";
    static char t_short[]            = "MPI_SHORT";
    static char t_ushort[]           = "MPI_UNSIGNED_SHORT";
    static char t_int[]              = "MPI_INT";
    static char t_uint[]             = "MPI_UNSIGNED";
    static char t_long[]             = "MPI_LONG";
    static char t_ulong[]            = "MPI_UNSIGNED_LONG";
    static char t_float[]            = "MPI_FLOAT";
    static char t_double[]           = "MPI_DOUBLE";
    static char t_longdouble[]       = "MPI_LONG_DOUBLE";
    static char t_longlongint[]      = "MPI_LONG_LONG_INT";
    static char t_longlong[]         = "MPI_LONG_LONG";
    static char t_ulonglong[]        = "MPI_UNSIGNED_LONG_LONG";
    static char t_schar[]            = "MPI_SIGNED_CHAR";

    static char t_packed[]           = "MPI_PACKED";
    static char t_lb[]               = "MPI_LB";
    static char t_ub[]               = "MPI_UB";

    static char t_floatint[]         = "MPI_FLOAT_INT";
    static char t_doubleint[]        = "MPI_DOUBLE_INT";
    static char t_longint[]          = "MPI_LONG_INT";
    static char t_shortint[]         = "MPI_SHORT_INT";
    static char t_2int[]             = "MPI_2INT";
    static char t_longdoubleint[]    = "MPI_LONG_DOUBLE_INT";

    static char t_complex[]          = "MPI_COMPLEX";
    static char t_doublecomplex[]    = "MPI_DOUBLE_COMPLEX";
    static char t_logical[]          = "MPI_LOGICAL";
    static char t_real[]             = "MPI_REAL";
    static char t_doubleprecision[]  = "MPI_DOUBLE_PRECISION";
    static char t_integer[]          = "MPI_INTEGER";
    static char t_2integer[]         = "MPI_2INTEGER";
#ifdef MPICH_DEFINE_2COMPLEX
    static char t_2complex[]         = "MPI_2COMPLEX";
    static char t_2doublecomplex[]   = "MPI_2DOUBLE_COMPLEX";
#endif
    static char t_2real[]            = "MPI_2REAL";
    static char t_2doubleprecision[] = "MPI_2DOUBLE_PRECISION";
    static char t_character[]        = "MPI_CHARACTER";

    if (type == MPI_CHAR)              return t_char;
    if (type == MPI_UNSIGNED_CHAR)     return t_uchar;
    if (type == MPI_SIGNED_CHAR)       return t_schar;
    if (type == MPI_BYTE)              return t_byte;
    if (type == MPI_WCHAR)             return t_wchar_t;
    if (type == MPI_SHORT)             return t_short;
    if (type == MPI_UNSIGNED_SHORT)    return t_ushort;
    if (type == MPI_INT)               return t_int;
    if (type == MPI_UNSIGNED)          return t_uint;
    if (type == MPI_LONG)              return t_long;
    if (type == MPI_UNSIGNED_LONG)     return t_ulong;
    if (type == MPI_FLOAT)             return t_float;
    if (type == MPI_DOUBLE)            return t_double;
    if (type == MPI_LONG_DOUBLE)       return t_longdouble;
    if (type == MPI_LONG_LONG_INT)     return t_longlongint;
    if (type == MPI_LONG_LONG)         return t_longlong;
    if (type == MPI_UNSIGNED_LONG_LONG) return t_ulonglong;
	
    if (type == MPI_PACKED)            return t_packed;
    if (type == MPI_LB)                return t_lb;
    if (type == MPI_UB)                return t_ub;
	
    if (type == MPI_FLOAT_INT)         return t_floatint;
    if (type == MPI_DOUBLE_INT)        return t_doubleint;
    if (type == MPI_LONG_INT)          return t_longint;
    if (type == MPI_SHORT_INT)         return t_shortint;
    if (type == MPI_2INT)              return t_2int;
    if (type == MPI_LONG_DOUBLE_INT)   return t_longdoubleint;
	
    if (type == MPI_COMPLEX)           return t_complex;
    if (type == MPI_DOUBLE_COMPLEX)    return t_doublecomplex;
    if (type == MPI_LOGICAL)           return t_logical;
    if (type == MPI_REAL)              return t_real;
    if (type == MPI_DOUBLE_PRECISION)  return t_doubleprecision;
    if (type == MPI_INTEGER)           return t_integer;
    if (type == MPI_2INTEGER)          return t_2integer;
#ifdef MPICH_DEFINE_2COMPLEX
    if (type == MPI_2COMPLEX)          return t_2complex;
    if (type == MPI_2DOUBLE_COMPLEX)   return t_2doublecomplex;
#endif
    if (type == MPI_2REAL)             return t_2real;
    if (type == MPI_2DOUBLE_PRECISION) return t_2doubleprecision;
    if (type == MPI_CHARACTER)         return t_character;

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
    static char c_named[]    = "named";
    static char c_contig[]   = "contig";
    static char c_vector[]   = "vector";
    static char c_hvector[]  = "hvector";
    static char c_indexed[]  = "indexed";
    static char c_hindexed[] = "hindexed";
    static char c_struct[]   = "struct";
    static char c_dup[]              = "dup";
    static char c_hvector_integer[]  = "hvector_integer";
    static char c_hindexed_integer[] = "hindexed_integer";
    static char c_indexed_block[]    = "indexed_block";
    static char c_hindexed_block[]   = "hindexed_block";
    static char c_struct_integer[]   = "struct_integer";
    static char c_subarray[]         = "subarray";
    static char c_darray[]           = "darray";
    static char c_f90_real[]         = "f90_real";
    static char c_f90_complex[]      = "f90_complex";
    static char c_f90_integer[]      = "f90_integer";
    static char c_resized[]          = "resized";

    if (combiner == MPI_COMBINER_NAMED)      return c_named;
    if (combiner == MPI_COMBINER_CONTIGUOUS) return c_contig;
    if (combiner == MPI_COMBINER_VECTOR)     return c_vector;
    if (combiner == MPI_COMBINER_HVECTOR)    return c_hvector;
    if (combiner == MPI_COMBINER_INDEXED)    return c_indexed;
    if (combiner == MPI_COMBINER_HINDEXED)   return c_hindexed;
    if (combiner == MPI_COMBINER_STRUCT)     return c_struct;
    if (combiner == MPI_COMBINER_DUP)              return c_dup;
    if (combiner == MPI_COMBINER_HVECTOR_INTEGER)  return c_hvector_integer;
    if (combiner == MPI_COMBINER_HINDEXED_INTEGER) return c_hindexed_integer;
    if (combiner == MPI_COMBINER_INDEXED_BLOCK)    return c_indexed_block;
    if (combiner == MPI_COMBINER_HINDEXED_BLOCK)   return c_hindexed_block;
    if (combiner == MPI_COMBINER_STRUCT_INTEGER)   return c_struct_integer;
    if (combiner == MPI_COMBINER_SUBARRAY)         return c_subarray;
    if (combiner == MPI_COMBINER_DARRAY)           return c_darray;
    if (combiner == MPI_COMBINER_F90_REAL)         return c_f90_real;
    if (combiner == MPI_COMBINER_F90_COMPLEX)      return c_f90_complex;
    if (combiner == MPI_COMBINER_F90_INTEGER)      return c_f90_integer;
    if (combiner == MPI_COMBINER_RESIZED)          return c_resized;

    return NULL;
}

/* --BEGIN DEBUG-- */
/*
 * You must configure MPICH2 with the logging option enabled (--enable-g=log)
 * for these routines to print - in which case, they use the same options
 * as the logging code, including print to file and control by class (DATATYPE)
 */
void MPIR_Datatype_debug(MPI_Datatype type,
			  int array_ct)
{
    int is_builtin;
    MPIR_Datatype *dtp ATTRIBUTE((unused));

    is_builtin = (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN);

    /* can get a NULL type a number of different ways, including not having
     * fortran support included.
     */
    if (type == MPI_DATATYPE_NULL) {
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,
			 (MPL_DBG_FDEST,
			  "# MPIU_Datatype_debug: MPI_Datatype = MPI_DATATYPE_NULL"));
	return;
    }

    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	       "# MPIU_Datatype_debug: MPI_Datatype = 0x%0x (%s)", type,
	       (is_builtin) ? MPIR_Datatype_builtin_to_string(type) :
	        "derived"));

    if (is_builtin) return;

    MPIR_Datatype_get_ptr(type, dtp);
    MPIR_Assert(dtp != NULL);

    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
      "# Size = " MPI_AINT_FMT_DEC_SPEC ", Extent = " MPI_AINT_FMT_DEC_SPEC ", LB = " MPI_AINT_FMT_DEC_SPEC "%s, UB = " MPI_AINT_FMT_DEC_SPEC "%s, Extent = " MPI_AINT_FMT_DEC_SPEC ", Element Size = " MPI_AINT_FMT_DEC_SPEC " (%s), %s",
		    (MPI_Aint) dtp->size,
		    (MPI_Aint) dtp->extent,
		    (MPI_Aint) dtp->lb,
		    (dtp->has_sticky_lb) ? "(sticky)" : "",
		    (MPI_Aint) dtp->ub,
		    (dtp->has_sticky_ub) ? "(sticky)" : "",
		    (MPI_Aint) dtp->extent,
		    (MPI_Aint) dtp->builtin_element_size,
		    dtp->builtin_element_size == -1 ? "multiple types" :
		    MPIR_Datatype_builtin_to_string(dtp->basic_type),
		    dtp->is_contig ? "is N contig" : "is not N contig"));

    MPL_DBG_OUT(MPIR_DBG_DATATYPE,"# Contents:");
    MPII_Datatype_contents_printf(type, 0, array_ct);

    MPL_DBG_OUT(MPIR_DBG_DATATYPE,"# Dataloop:");
    MPII_Datatype_dot_printf(type, 0, 1);
}

static char *MPII_Datatype_depth_spacing(int depth)
{
    static char d0[] = "";
    static char d1[] = "  ";
    static char d2[] = "    ";
    static char d3[] = "      ";
    static char d4[] = "        ";
    static char d5[] = "          ";

    switch (depth) {
	case 0: return d0;
	case 1: return d1;
	case 2: return d2;
	case 3: return d3;
	case 4: return d4;
	default: return d5;
    }
}

#define MPII_DATATYPE_FREE_AND_RETURN { \
 if (cp->nr_ints  > 0) MPL_free(ints);   \
 if (cp->nr_aints > 0) MPL_free(aints);   \
 if (cp->nr_types > 0) MPL_free(types);   \
 return;                                 }

void MPII_Datatype_contents_printf(MPI_Datatype type,
				    int depth,
				    int acount)
{
    int i;
    MPIR_Datatype *dtp;
    MPIR_Datatype_contents *cp;

    MPI_Aint *aints = NULL;
    MPI_Datatype *types = NULL;
    int *ints = NULL;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %stype: %s\n",
			MPII_Datatype_depth_spacing(depth),
			MPIR_Datatype_builtin_to_string(type)));
	return;
    }

    MPIR_Datatype_get_ptr(type, dtp);
    cp = dtp->contents;

    if (cp == NULL) {
	MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# <NULL>\n"));
	return;
    }

    if (cp->nr_ints > 0)
    {
      ints = (int *) MPL_malloc(cp->nr_ints * sizeof(int));
      MPII_Datatype_get_contents_ints(cp, ints);
    }

    if (cp->nr_aints > 0) {
      aints = (MPI_Aint *) MPL_malloc(cp->nr_aints * sizeof(MPI_Aint));
      MPII_Datatype_get_contents_aints(cp, aints);
    }

    if (cp->nr_types > 0) {
      types = (MPI_Datatype *) MPL_malloc(cp->nr_types * sizeof(MPI_Datatype));
      MPII_Datatype_get_contents_types(cp, types);
    }


    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %scombiner: %s",
		    MPII_Datatype_depth_spacing(depth),
		    MPIR_Datatype_combiner_to_string(cp->combiner)));

    switch (cp->combiner) {
	case MPI_COMBINER_NAMED:
	case MPI_COMBINER_DUP:
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_RESIZED:
	    /* not done */
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_CONTIGUOUS:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %scontig ct = %d\n",
			    MPII_Datatype_depth_spacing(depth),
				       *ints));
	    MPII_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_VECTOR:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	                "# %svector ct = %d, blk = %d, str = %d\n",
			MPII_Datatype_depth_spacing(depth),
			    ints[0],
			    ints[1],
			    ints[2]));
	    MPII_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    MPII_DATATYPE_FREE_AND_RETURN;
        case MPI_COMBINER_HVECTOR:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
	                  "# %shvector ct = %d, blk = %d, str = " MPI_AINT_FMT_DEC_SPEC "\n",
			    MPII_Datatype_depth_spacing(depth),
			    ints[0],
			    ints[1],
			    (MPI_Aint) aints[0]));
	    MPII_Datatype_contents_printf(*types,
					   depth + 1,
					   acount);
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_INDEXED:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %sindexed ct = %d:",
			    MPII_Datatype_depth_spacing(depth),
			    ints[0]));
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		         "# %s  indexed [%d]: blk = %d, disp = %d\n",
				MPII_Datatype_depth_spacing(depth),
				i,
				ints[i+1],
				ints[i+(cp->nr_ints/2)+1]));
		MPII_Datatype_contents_printf(*types,
					       depth + 1,
					       acount);
	    }
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_HINDEXED:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %shindexed ct = %d:",
			    MPII_Datatype_depth_spacing(depth),
			    ints[0]));
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		            "# %s  hindexed [%d]: blk = %d, disp = " MPI_AINT_FMT_DEC_SPEC "\n",
				MPII_Datatype_depth_spacing(depth),
				i,
				(int) ints[i+1],
				(MPI_Aint) aints[i]));
		MPII_Datatype_contents_printf(*types,
					       depth + 1,
					       acount);
	    }
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_STRUCT:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %sstruct ct = %d:",
			    MPII_Datatype_depth_spacing(depth),
			    (int) ints[0]));
	    for (i=0; i < acount && i < ints[0]; i++) {
		MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
		           "# %s  struct[%d]: blk = %d, disp = " MPI_AINT_FMT_DEC_SPEC "\n",
				MPII_Datatype_depth_spacing(depth),
				i,
				(int) ints[i+1],
				(MPI_Aint) aints[i]));
		MPII_Datatype_contents_printf(types[i],
					       depth + 1,
					       acount);
	    }
	    MPII_DATATYPE_FREE_AND_RETURN;
	case MPI_COMBINER_SUBARRAY:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,"# %ssubarray ct = %d:",
			MPII_Datatype_depth_spacing(depth),
			(int) ints[0]));
	    for (i=0; i< acount && i < ints[0]; i++) {
		MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,
			    "# %s  sizes[%d] = %d subsizes[%d] = %d starts[%d] = %d\n",
			    MPII_Datatype_depth_spacing(depth),
			    i, (int)ints[i+1],
			    i, (int)ints[i+ ints[0]+1],
			    i, (int)ints[2*ints[0]+1]));
	    }
	    MPII_Datatype_contents_printf(*types,
		    depth + 1,
		    acount);
	    MPII_DATATYPE_FREE_AND_RETURN;

	default:
	    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE,(MPL_DBG_FDEST,"# %sunhandled combiner",
			MPII_Datatype_depth_spacing(depth)));
	    MPII_DATATYPE_FREE_AND_RETURN;
    }
}
/* --END DEBUG-- */

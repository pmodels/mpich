/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Equal structures */
/* For reusing code, using int type instead of bool*/
typedef struct MPIR_2int_eqltype {
    int value;
    int is_equal;
} MPIR_2int_eqltype;

typedef struct MPIR_floatint_eqltype {
    float value;
    int is_equal;
} MPIR_floatint_eqltype;

typedef struct MPIR_longint_eqltype {
    long value;
    int is_equal;
} MPIR_longint_eqltype;

typedef struct MPIR_shortint_eqltype {
    short value;
    int is_equal;
} MPIR_shortint_eqltype;

typedef struct MPIR_doubleint_eqltype {
    double value;
    int is_equal;
} MPIR_doubleint_eqltype;

#if defined(HAVE_LONG_DOUBLE)
typedef struct MPIR_longdoubleint_eqltype {
    long double value;
    int is_equal;
} MPIR_longdoubleint_eqltype;
#endif

#define MPIR_EQUAL_FLOAT_PRECISION 1e-6

#define MPIR_EQUAL_FLOAT_COMPARE(_aValue, _bValue)              \
    (((_aValue <= _bValue + MPIR_EQUAL_FLOAT_PRECISION)         \
    && (_aValue >= _bValue - MPIR_EQUAL_FLOAT_PRECISION))?      \
    1:0)

/* If a child found unequal, its parent sticks to unequal. */
/* Values of is_equal: 1 equal 0 not equal, init using any value other than 0.*/
#define MPIR_EQUAL_C_CASE_INT(c_type_) {                \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i = 0; i < len; ++i) {                         \
            if (0 == b[i].is_equal || 0 == a[i].is_equal){    \
                a[i].is_equal = 0;            			\
            }else if (a[i].value != b[i].value){        \
                a[i].is_equal = 0;                       \
            }else{                                      \
                a[i].is_equal = 1;                       \
            }                                           \
        }                                               \
    }                                                   \
    break

#define MPIR_EQUAL_C_CASE_FLOAT(c_type_) {              \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i = 0; i < len; ++i) {                         \
            if (0 == b[i].is_equal || 0 == a[i].is_equal){    \
                a[i].is_equal = 0;            			\
            }else if (!MPIR_EQUAL_FLOAT_COMPARE(a[i].value, b[i].value)){        \
                a[i].is_equal = 0;                       \
            }else{                                      \
                a[i].is_equal = 1;                       \
            }                                           \
        }                                               \
    }                                                   \
    break

#define MPIR_EQUAL_F_CASE(f_type_) {                    \
        f_type_ *a = (f_type_ *)inoutvec;               \
        f_type_ *b = (f_type_ *)invec;                  \
        for (i = 0; i < flen; i += 2) {                       \
            if(0 == b[i+1] || 0 == a[i+1]){             \
                a[i+1] = 0;                        		\
            }else if (a[i] != b[i]){                    \
                a[i+1] = 0;                             \
            }else{                                      \
                a[i+1] = 1;                             \
            }                                           \
        }                                               \
    }                                                   \
    break

void MPIR_EQUAL_user_defined_datatype_compare(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
	int mpi_errno = MPI_SUCCESS;
	int i, j, size, len = *Len, is_equal; 
	int data_len = 0, type_len = 0;
	int num_ints, num_adds, num_types, combiner, *ints;
	void *invec_i_pos, *invec_i_bool_pos, *inoutvec_i_pos, *inoutvec_i_bool_pos;
	MPI_Aint *adds = NULL;
	MPI_Aint lb, extent;
	MPI_Datatype *types;

	 /* decode */
	MPIR_Type_get_envelope(*type, &num_ints, &num_adds, &num_types, &combiner);

	if(num_types < 2 || combiner != MPI_COMBINER_STRUCT) /*At least 2 elements is required, data, result*/
	{
		MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPIX_EQUAL");
		return;
	}

	ints = (int *)malloc(num_ints * sizeof(*ints));

	if (num_adds || (num_ints != num_adds+1))/*ints[0] is the length*/
	{
	    adds = (MPI_Aint *)malloc(num_adds * sizeof(*adds));
	}
	else /*adds is required to avoid impacts of struct alignment*/
	{
		MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPIX_EQUAL");
		return;
	}

	types = (MPI_Datatype *)malloc(num_types * sizeof(*types));

	mpi_errno = MPIR_Type_get_contents(*type, num_ints, num_adds, num_types, ints, adds, types);

	if(types[num_types-1] != MPI_INT) /*The last element has to be int*/
	{
		MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPIX_EQUAL");
		return;
	}

	MPIR_Type_get_extent_impl(*type, &lb, &extent);
	type_len = extent - lb;

	for (i = 0; i < len; ++i)    	
	{
		invec_i_pos = invec + i*type_len;
		invec_i_bool_pos = invec_i_pos + adds[num_adds-1];
		inoutvec_i_pos = inoutvec + i*type_len;
		inoutvec_i_bool_pos = inoutvec_i_pos + adds[num_adds-1];
		if((*(int*)invec_i_bool_pos == 0) ||
			(*(int*)inoutvec_i_bool_pos == 0))
		{
			*(int*)inoutvec_i_bool_pos  = 0;
		}
		else
		{/*compare the content of the struct*/
			is_equal = 1;
			for(j = 0; j < num_adds-1; ++j)
			{
				MPI_Type_size(types[j], &size); 
				data_len = ints[j + 1] * size;

				if(memcmp(invec_i_pos+adds[j], inoutvec_i_pos+adds[j], data_len))
					is_equal = 0;
			}

			*(int*)inoutvec_i_bool_pos  = is_equal;
		}
	}
}

void MPIR_EQUAL(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
    int i, len = *Len;

#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
    int flen = len * 2;         /* used for Fortran types */
#endif
#endif
    
    switch (*type) {
            /* first the C types */
        case MPI_2INT:
            MPIR_EQUAL_C_CASE_INT(MPIR_2int_eqltype);
        case MPI_FLOAT_INT:
            MPIR_EQUAL_C_CASE_FLOAT(MPIR_floatint_eqltype);
        case MPI_LONG_INT:
            MPIR_EQUAL_C_CASE_INT(MPIR_longint_eqltype);
        case MPI_SHORT_INT:
            MPIR_EQUAL_C_CASE_INT(MPIR_shortint_eqltype);
        case MPI_DOUBLE_INT:
            MPIR_EQUAL_C_CASE_FLOAT(MPIR_doubleint_eqltype);
#if defined(HAVE_LONG_DOUBLE)
        case MPI_LONG_DOUBLE_INT:
            MPIR_EQUAL_C_CASE_FLOAT(MPIR_longdoubleint_eqltype);
#endif

            /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
        case MPI_2INTEGER:
            MPIR_EQUAL_F_CASE(MPI_Fint);
#endif
#endif
        default:
            //MPIR_Assert(0);
            MPIR_EQUAL_user_defined_datatype_compare(invec, inoutvec, Len, type);
            break;
    }

}


int MPIR_EQUAL_check_dtype(MPI_Datatype type)
{
	//To support user defined datatypes, no actual type check now.
    int mpi_errno = MPI_SUCCESS;

    switch (type) {
            /* first the C types */
        case MPI_2INT:
        case MPI_FLOAT_INT:
        case MPI_LONG_INT:
        case MPI_SHORT_INT:
        case MPI_DOUBLE_INT:
#if defined(HAVE_LONG_DOUBLE)
        case MPI_LONG_DOUBLE_INT:
#endif
            /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
        case MPI_2INTEGER:
#endif
#endif
            break;

        default:
        	break;
            //MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_EQUAL");
    }

    return mpi_errno;
}

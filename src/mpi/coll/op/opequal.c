/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Equal structures */
/* For reusing code, using int type instead of bool*/
typedef struct MPIR_2int_eqltype {
    int value;
    int isEqual;
} MPIR_2int_eqltype;

typedef struct MPIR_floatint_eqltype {
    float value;
    int isEqual;
} MPIR_floatint_eqltype;

typedef struct MPIR_longint_eqltype {
    long value;
    int isEqual;
} MPIR_longint_eqltype;

typedef struct MPIR_shortint_eqltype {
    short value;
    int isEqual;
} MPIR_shortint_eqltype;

typedef struct MPIR_doubleint_eqltype {
    double value;
    int isEqual;
} MPIR_doubleint_eqltype;

#if defined(HAVE_LONG_DOUBLE)
typedef struct MPIR_longdoubleint_eqltype {
    long double value;
    int isEqual;
} MPIR_longdoubleint_eqltype;
#endif

#define MPIR_EQUAL_FLOAT_PRECISION 1e-6

#define C_STRUCT_PADDING(_size) (_size = (_size%8) ? (_size - _size%8 + 8) : _size)

#define MPIR_EQUAL_FLOAT_COMPARE(_aValue, _bValue)              \
    (((_aValue <= _bValue + MPIR_EQUAL_FLOAT_PRECISION)         \
    && (_aValue >= _bValue - MPIR_EQUAL_FLOAT_PRECISION))?      \
    1:0)


/* If a child found unequal, its parent sticks to unequal. */
/* Values of isEqual: 1 equal 0 not equal, init using any value other than 0.*/
#define MPIR_EQUAL_C_CASE_INT(c_type_) {                \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i = 0; i < len; ++i) {                         \
            if (0 == b[i].isEqual || 0 == a[i].isEqual){    \
                a[i].isEqual = 0;            			\
            }else if (a[i].value != b[i].value){        \
                a[i].isEqual = 0;                       \
            }else{                                      \
                a[i].isEqual = 1;                       \
            }                                           \
        }                                               \
    }                                                   \
    break

#define MPIR_EQUAL_C_CASE_FLOAT(c_type_) {              \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i = 0; i < len; ++i) {                         \
            if (0 == b[i].isEqual || 0 == a[i].isEqual){    \
                a[i].isEqual = 0;            			\
            }else if (!MPIR_EQUAL_FLOAT_COMPARE(a[i].value, b[i].value)){        \
                a[i].isEqual = 0;                       \
            }else{                                      \
                a[i].isEqual = 1;                       \
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
	int i, j, size, len = *Len; 
	int data_len = 0;
	int type_len = 0, struct_len = 0;
	int num_ints, num_adds, num_types, combiner, *ints;
    MPI_Aint *adds = NULL;
    MPI_Datatype *types;

	 /* decode */
    mpi_errno = MPI_Type_get_envelope(*type, &num_ints, &num_adds, &num_types, &combiner);

    if(num_types < 3 || (combiner != MPI_COMBINER_STRUCT && combiner != MPI_COMBINER_VECTOR)) /*At least 2 elements is required, data, result*/
	{
		MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_EQUAL");
		return;
	}

	ints = (int *)malloc(num_ints * sizeof(*ints));
    if (num_adds)
        adds = (MPI_Aint *)malloc(num_adds * sizeof(*adds));
    types = (MPI_Datatype *)malloc(num_types * sizeof(*types));

    mpi_errno = MPI_Type_get_contents(*type, num_ints, num_adds, num_types, ints, adds, types);

    if(types[num_types-1] != MPI_INT) /*The last element has to be int*/
	{
		MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_EQUAL");
		return;
	}

    for(i = 0; i < num_types; ++i)
    {
    	MPI_Type_size(types[i], &size); 
    	type_len += size;

    }

    struct_len = type_len;

    if(combiner == MPI_COMBINER_STRUCT)
    	C_STRUCT_PADDING(struct_len);

    for (i = 0; i < len; ++i)
    {
    	if(*(int*)(invec + i*struct_len + type_len - sizeof(int)) == 0 ||
    		*(int*)(inoutvec + i*struct_len + type_len - sizeof(int)) == 0)
    	{
    		*(int*)(inoutvec + i*struct_len + type_len - sizeof(int)) = 0;
    	}
    	else if(memcmp(invec + i*struct_len, 
    			inoutvec + i*struct_len, type_len - sizeof(int)))
    	{
    		*(int*)(inoutvec + i*struct_len + type_len - sizeof(int)) = 0;
    	}
    	else
    	{
    		*(int*)(inoutvec + i*struct_len + type_len - sizeof(int)) = 1;
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
        case MPI_2REAL:
            MPIR_EQUAL_F_CASE(MPIR_FC_REAL_CTYPE);
        case MPI_2DOUBLE_PRECISION:
            MPIR_EQUAL_F_CASE(MPIR_FC_DOUBLE_CTYPE);
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
	//To support user defined datatypes, no type check now.
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
        case MPI_2REAL:
        case MPI_2DOUBLE_PRECISION:
#endif
#endif
            break;

        default:
        	break;
            //MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_EQUAL");
    }

    return mpi_errno;
}

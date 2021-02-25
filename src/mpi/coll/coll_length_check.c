/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#define MPI_DATATYPE_HASH_UNDEFINED 0xFFFFFFFF
#define CHAR_BIT 8

typedef struct Type_Sig {
    unsigned long num_types;
    unsigned long hash_value;   /* used in hash generator */
} Type_Sig;

//Example 1001 << 1 -> 0011
unsigned long MPIR_Circular_Left_Shift(unsigned long val, unsigned long nums_left_shift)
{
    unsigned long return_val;

    return_val = (val << nums_left_shift) | 
    		(val >> (sizeof(val)*CHAR_BIT - nums_left_shift));

    return return_val;
}

//(a, n) op (b,m) = (a XOR (B Circular_Left_Shift n), n+m)
Type_Sig MPIR_Datatype_Sig_Hash_Generator(Type_Sig  type_sig_a, Type_Sig type_sig_b)
{
    Type_Sig  type_sig_to_return;
    unsigned long temp_hash_value;

    type_sig_to_return.num_types = type_sig_a.num_types + type_sig_b.num_types;

    temp_hash_value = MPIR_Circular_Left_Shift(type_sig_b.hash_value, type_sig_a.num_types);

    type_sig_to_return.hash_value = type_sig_b.hash_value ^ temp_hash_value;

    return type_sig_to_return;
}

//Generate the hash info for basic MPI_Datatypes
Type_Sig MPIR_Datatype_Init_Hash_Generator(MPI_Datatype type)
{
	Type_Sig temp_hash;
	temp_hash.num_types = 1;

	HASH_VALUE(&type, sizeof(MPI_Datatype), temp_hash.hash_value);
	
	return temp_hash;
}
/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#define MPI_DATATYPE_HASH_UNDEFINED (0b10101010<<8) + 0b00000000

typedef struct type_hash {
    MPI_Datatype type;
    const unsigned int hash_value;     /* used in hash generator */
} type_hash;

/* -- Define initial hash values for MPI basic datatypes */
static type_hash mpi_type_hashs[] = {
    {MPI_CHAR, (0b10101010<<8) + 0b00000001},
    {MPI_SHORT, (0b10101010<<8) + 0b00000010},
    {MPI_INT, (0b10101010<<8) + 0b00000011},
    {MPI_LONG, (0b10101010<<8) + 0b00000100},
    {MPI_LONG_LONG, (0b10101010<<8) + 0b00000101},
    {MPI_SIGNED_CHAR, (0b10101010<<8) + 0b00000110},
    {MPI_UNSIGNED_CHAR, (0b10101010<<8) + 0b00000111},
    {MPI_UNSIGNED_SHORT, (0b10101010<<8) + 0b00001000},
    {MPI_UNSIGNED, (0b10101010<<8) + 0b00001001},
    {MPI_UNSIGNED_LONG, (0b10101010<<8) + 0b00001010},
    {MPI_UNSIGNED_LONG_LONG, (0b10101010<<8) + 0b00001011},
    {MPI_FLOAT, (0b10101010<<8) + 0b00001100},
    {MPI_DOUBLE, (0b10101010<<8) + 0b00001101},
    {MPI_LONG_DOUBLE, (0b10101010<<8) + 0b00001110},
    {MPI_COMPLEX, (0b10101010<<8) + 0b00001111},
    {MPI_DOUBLE_COMPLEX, (0b10101010<<8) + 0b00010000},
    {MPI_LONG_DOUBLE_COMPLEX    , (0b10101010<<8) + 0b00010001},
    {MPI_WCHAR, (0b10101010<<8) + 0b00010010},
    {MPI_BYTE, (0b10101010<<8) + 0b00010011},
    {MPI_PACKED, (0b10101010<<8) + 0b00100001}
    //TODO fortran datatypes   
};


//Get the initial hash value for a MPI_Datatype
MPI_Datatype MPIR_Datatype_Hash_builtin_search_by_datatype(MPI_Datatype datatype)
{
    int i;
    unsigned int  hash_value_to_return= MPI_DATATYPE_HASH_UNDEFINED;
    for (i = 0; i < sizeof(mpi_type_hashs) / sizeof(type_hash); ++i) {
        if (mpi_type_hashs[i].type == datatype){
            hash_value_to_return = mpi_type_hashs[i].hash_value;
            break;
        }
    }
    return hash_value_to_return;
}

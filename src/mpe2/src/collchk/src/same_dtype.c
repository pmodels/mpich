/*
   (C) 2004 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "collchk.h" 

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#else
# if defined( HAVE_ALLOCA_H )
# include <alloca.h>
# endif
#endif


unsigned int CollChk_cirleftshift( unsigned int alpha, unsigned n );
unsigned int CollChk_cirleftshift( unsigned int alpha, unsigned n )
{
    /* Doing circular left shift of alpha by n bits */
    unsigned int t1, t2;
    t1 = alpha >> (sizeof(unsigned int)-n);
    t2 = alpha << n;
    return t1 | t2;
}

void CollChk_hash_add(const CollChk_hash_t *alpha,
                      const CollChk_hash_t *beta,
                            CollChk_hash_t *lamda);
void CollChk_hash_add(const CollChk_hash_t *alpha,
                      const CollChk_hash_t *beta,
                            CollChk_hash_t *lamda)
{
    lamda->value = (alpha->value)
                 ^ CollChk_cirleftshift(beta->value, alpha->count);
    lamda->count = alpha->count + beta->count;
}

int CollChk_hash_equal(const CollChk_hash_t *alpha,
                       const CollChk_hash_t *beta)
{
    return alpha->count == beta->count && alpha->value == beta->value;
}

#if defined( HAVE_LAM_FORTRAN_MPI_DATATYPE_IN_C )
#include "lam_f2c_dtype.h"
#endif

unsigned int CollChk_basic_value(MPI_Datatype type);
unsigned int CollChk_basic_value(MPI_Datatype type)
{
    /*
       MPI_Datatype's that return 0x0 are as if they are being
       skipped/ignored in the comparison of any 2 MPI_Datatypes.
    */
    if ( type == MPI_DATATYPE_NULL || type == MPI_UB || type == MPI_LB )
        return 0x0;
    else if ( type == MPI_CHAR )
        return 0x1;
#if defined( HAVE_MPI_SIGNED_CHAR )
    else if ( type == MPI_SIGNED_CHAR )
        return 0x3;
#endif
    else if ( type == MPI_UNSIGNED_CHAR )
        return 0x5;
    else if ( type == MPI_BYTE )
        return 0x7;
#if defined( HAVE_MPI_WCHAR )
    else if ( type == MPI_WCHAR )
        return 0x9;
#endif
    else if ( type == MPI_SHORT )
        return 0xb;
    else if ( type == MPI_UNSIGNED_SHORT )
        return 0xd;
    else if ( type == MPI_INT )
        return 0xf;
    else if ( type == MPI_UNSIGNED )
        return 0x11;
    else if ( type == MPI_LONG )
        return 0x13;
    else if ( type == MPI_UNSIGNED_LONG )
        return 0x15;
    else if ( type == MPI_FLOAT )
        return 0x17;
    else if ( type == MPI_DOUBLE )
        return 0x19;
    else if ( type == MPI_LONG_DOUBLE )
        return 0x1b;
    else if ( type == MPI_LONG_LONG_INT )
        return 0x1d;
    /* else if ( type == MPI_LONG_LONG ) return 0x1f; */
#if defined( HAVE_MPI_UNSIGNED_LONG_LONG )
    else if ( type == MPI_UNSIGNED_LONG_LONG )
        return 0x21;
#endif

    else if ( type == MPI_FLOAT_INT )
        return 0x8;       /* (0x17,1)@(0xf,1) */
    else if ( type == MPI_DOUBLE_INT )
        return 0x6;       /* (0x19,1)@(0xf,1) */
    else if ( type == MPI_LONG_INT )
        return 0xc;       /* (0x13,1)@(0xf,1) */
    else if ( type == MPI_SHORT_INT )
        return 0x14;      /* (0xb,1)@(0xf,1) */
    else if ( type == MPI_2INT )
        return 0x10;      /* (0xf,1)@(0xf,1) */
    else if ( type == MPI_LONG_DOUBLE_INT )
        return 0x4;       /* (0x1b,1)@(0xf,1) */

#if defined( HAVE_FORTRAN_MPI_DATATYPE_IN_C )
    else if ( type == MPI_COMPLEX )
        return 0x101;
    else if ( type == MPI_DOUBLE_COMPLEX )
        return 0x103;
    else if ( type == MPI_LOGICAL )
        return 0x105;
    else if ( type == MPI_REAL )
        return 0x107;
    else if ( type == MPI_DOUBLE_PRECISION )
        return 0x109;
    else if ( type == MPI_INTEGER )
        return 0x10b;
    else if ( type == MPI_CHARACTER )
        return 0x10d;

    else if ( type == MPI_2INTEGER )
        return 0x33c;      /* (0x10b,1)@(0x10b,1) */
    else if ( type == MPI_2REAL )
        return 0x329;      /* (0x107,1)@(0x107,1) */
    else if ( type == MPI_2DOUBLE_PRECISION )
        return 0x33a;      /* (0x109,1)@(0x109,1) */
#if defined( HAVE_FORTRAN_MPI_DATATYPE_2COMPLEX_IN_C )
    else if ( type == MPI_2COMPLEX )
        return 0x323;      /* (0x101,1)@(0x101,1) */
    else if ( type == MPI_2DOUBLE_COMPLEX )
        return 0x325;      /* (0x103,1)@(0x103,1) */
#endif

    else if ( type == MPI_PACKED )
        return 0x201;
#if defined( HAVE_FORTRAN_MPI_DATATYPE_INTEGERX_IN_C )
    else if ( type == MPI_INTEGER1 )
        return 0x211;
    else if ( type == MPI_INTEGER2 )
        return 0x213;
    else if ( type == MPI_INTEGER4 )
        return 0x215;
    else if ( type == MPI_INTEGER8 )
        return 0x217;
    /* else if ( type == MPI_INTEGER16 ) return 0x219; */
#endif
#if defined( HAVE_FORTRAN_MPI_DATATYPE_REALX_IN_C )
    else if ( type == MPI_REAL4 )
        return 0x221;
    else if ( type == MPI_REAL8 )
        return 0x223;
    /* else if ( type == MPI_REAL16 ) return 0x205; */
#endif
#if defined( HAVE_FORTRAN_MPI_DATATYPE_COMPLEXX_IN_C )
    else if ( type == MPI_COMPLEX8 )
        return 0x231;
    else if ( type == MPI_COMPLEX16 )
        return 0x233;
    /* else if ( type == MPI_COMPLEX32 ) return 0x20b; */
#endif

#endif

    else {
#if defined( HAVE_INT_MPI_DATATYPE )
        fprintf( stderr, "CollChk_basic_value()) "
                         "Unknown basic MPI datatype %x.\n", type );
#elif defined( HAVE_PTR_MPI_DATATYPE )
        fprintf( stderr, "CollChk_basic_value()) "
                         "Unknown basic MPI datatype %p.\n", type );
#else
        fprintf( stderr, "CollChk_basic_value()) "
                         "Unknown basic MPI datatype.\n" );
#endif
        fflush( stderr );
        return 0;
    }
}

unsigned int CollChk_basic_count(MPI_Datatype type);
unsigned int CollChk_basic_count(MPI_Datatype type)
{
    /* MPI_Datatype's that return 0 are being skipped/ignored. */
    if (    type == MPI_DATATYPE_NULL
         || type == MPI_UB
         || type == MPI_LB
    ) return 0;

    else if (    type == MPI_CHAR
#if defined( HAVE_MPI_SIGNED_CHAR )
              || type == MPI_SIGNED_CHAR
#endif
              || type == MPI_UNSIGNED_CHAR
              || type == MPI_BYTE
#if defined( HAVE_MPI_WCHAR )
              || type == MPI_WCHAR
#endif
              || type == MPI_SHORT
              || type == MPI_UNSIGNED_SHORT
              || type == MPI_INT
              || type == MPI_UNSIGNED
              || type == MPI_LONG
              || type == MPI_UNSIGNED_LONG
              || type == MPI_FLOAT
              || type == MPI_DOUBLE
              || type == MPI_LONG_DOUBLE
              || type == MPI_LONG_LONG_INT
              /* || type == MPI_LONG_LONG */
#if defined( HAVE_MPI_UNSIGNED_LONG_LONG )
              || type == MPI_UNSIGNED_LONG_LONG
#endif
    ) return 1;

    else if (    type == MPI_FLOAT_INT
              || type == MPI_DOUBLE_INT
              || type == MPI_LONG_INT
              || type == MPI_SHORT_INT
              || type == MPI_2INT
              || type == MPI_LONG_DOUBLE_INT
    ) return 2;

#if defined( HAVE_FORTRAN_MPI_DATATYPE_IN_C )
    else if (    type == MPI_COMPLEX
              || type == MPI_DOUBLE_COMPLEX
              || type == MPI_LOGICAL
              || type == MPI_REAL
              || type == MPI_DOUBLE_PRECISION
              || type == MPI_INTEGER
              || type == MPI_CHARACTER
    ) return 1;

    else if (    type == MPI_2INTEGER
              || type == MPI_2REAL
              || type == MPI_2DOUBLE_PRECISION
#if defined( HAVE_FORTRAN_MPI_DATATYPE_2COMPLEX_IN_C )
              || type == MPI_2COMPLEX
              || type == MPI_2DOUBLE_COMPLEX
#endif
    ) return 2;

    else if (    type == MPI_PACKED
#if defined( HAVE_FORTRAN_MPI_DATATYPE_INTEGERX_IN_C )
              || type == MPI_INTEGER1
              || type == MPI_INTEGER2
              || type == MPI_INTEGER4
              || type == MPI_INTEGER8
        /* || type == MPI_INTEGER16 */
#endif
#if defined( HAVE_FORTRAN_MPI_DATATYPE_REALX_IN_C )
              || type == MPI_REAL4
              || type == MPI_REAL8
        /* || type == MPI_REAL16 */
#endif
#if defined( HAVE_FORTRAN_MPI_DATATYPE_COMPLEXX_IN_C )
              || type == MPI_COMPLEX8
              || type == MPI_COMPLEX16
        /* || type == MPI_COMPLEX32 */
#endif
    ) return 1;
#endif

    else {
#if defined( HAVE_INT_MPI_DATATYPE )
        fprintf( stderr, "CollChk_basic_count(): "
                         "Unknown basic MPI datatype %x.\n", type );
#elif defined( HAVE_PTR_MPI_DATATYPE )
        fprintf( stderr, "CollChk_basic_count(): "
                         "Unknown basic MPI datatype %p.\n", type );
#else
        fprintf( stderr, "CollChk_basic_count(): "
                         "Unknown basic MPI datatype.\n" );
#endif
        fflush( stderr );
        return 0;
    }
}


int CollChk_derived_count(int idx, int *ints, int combiner);
int CollChk_derived_count(int idx, int *ints, int combiner)
{
    int ii, tot_cnt;
#if defined( HAVE_RARE_MPI_COMBINERS )
    int dim_A, dim_B;
#endif
    
    tot_cnt = 0;
    switch(combiner) {
#if defined( HAVE_RARE_MPI_COMBINERS )
        case MPI_COMBINER_DUP : 
        case MPI_COMBINER_F90_REAL :
        case MPI_COMBINER_F90_COMPLEX :
        case MPI_COMBINER_F90_INTEGER :
        case MPI_COMBINER_RESIZED :
            return 1;
#endif

        case MPI_COMBINER_CONTIGUOUS :
            return ints[0];

#if defined( HAVE_RARE_MPI_COMBINERS )
        case MPI_COMBINER_HVECTOR_INTEGER :
        case MPI_COMBINER_INDEXED_BLOCK :
#endif
        case MPI_COMBINER_VECTOR :
        case MPI_COMBINER_HVECTOR :
            return ints[0]*ints[1];

#if defined( HAVE_RARE_MPI_COMBINERS )
        case MPI_COMBINER_HINDEXED_INTEGER :
#endif
        case MPI_COMBINER_INDEXED :
        case MPI_COMBINER_HINDEXED :
            for ( ii = ints[0]; ii > 0; ii-- ) {
                 tot_cnt += ints[ ii ];
            }
            return tot_cnt;

#if defined( HAVE_RARE_MPI_COMBINERS )
        case MPI_COMBINER_STRUCT_INTEGER :
#endif
        case MPI_COMBINER_STRUCT :
            return ints[idx+1];

#if defined( HAVE_RARE_MPI_COMBINERS )
        case MPI_COMBINER_SUBARRAY :
            dim_A   = ints[ 0 ] + 1;
            dim_B   = 2 * ints[ 0 ];
            for ( ii=dim_A; ii<=dim_B; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
        case MPI_COMBINER_DARRAY :
            for ( ii=3; ii<=ints[2]+2; ii++ ) {
                tot_cnt += ints[ ii ];
            }
            return tot_cnt;
#endif
    }
    return tot_cnt;
}


void CollChk_dtype_hash(MPI_Datatype type, int cnt, CollChk_hash_t *type_hash)
{
    int             nints, naddrs, ntypes, combiner;
    int             *ints; 
    MPI_Aint        *addrs; 
    MPI_Datatype    *types;
    CollChk_hash_t  curr_hash, next_hash;
    int             type_cnt;
    int             ii;

    /*  Don't know if this makes sense or not */
    if ( cnt <= 0 ) {
        /* (value,count)=(0,0) => skipping of this (type,cnt) in addition */
        type_hash->value = 0;
        type_hash->count = 0;
        return;
    }

    MPI_Type_get_envelope(type, &nints, &naddrs, &ntypes, &combiner);
    if (combiner != MPI_COMBINER_NAMED) {
#if ! defined( HAVE_ALLOCA )
        ints = NULL;
        if ( nints > 0 )
            ints = (int *) malloc(nints * sizeof(int)); 
        addrs = NULL;
        if ( naddrs > 0 )
            addrs = (MPI_Aint *) malloc(naddrs * sizeof(MPI_Aint)); 
        types = NULL;
        if ( ntypes > 0 )
            types = (MPI_Datatype *) malloc(ntypes * sizeof(MPI_Datatype));
#else
        ints = NULL;
        if ( nints > 0 )
            ints = (int *) alloca(nints * sizeof(int)); 
        addrs = NULL;
        if ( naddrs > 0 )
            addrs = (MPI_Aint *) alloca(naddrs * sizeof(MPI_Aint)); 
        types = NULL;
        if ( ntypes > 0 )
            types = (MPI_Datatype *) alloca(ntypes * sizeof(MPI_Datatype));
#endif

        MPI_Type_get_contents(type, nints, naddrs, ntypes, ints, addrs, types);
        type_cnt = CollChk_derived_count(0, ints, combiner);
        CollChk_dtype_hash(types[0], type_cnt, &curr_hash);

        /*
            ntypes > 1 only for MPI_COMBINER_STRUCT(_INTEGER)
        */
        for( ii=1; ii < ntypes; ii++) {
            type_cnt = CollChk_derived_count(ii, ints, combiner); 
            CollChk_dtype_hash(types[ii], type_cnt, &next_hash);
            CollChk_hash_add(&curr_hash, &next_hash, &curr_hash);
        }

#if ! defined( HAVE_ALLOCA )
        if ( ints != NULL )
            free( ints );
        if ( addrs != NULL )
            free( addrs );
        if ( types != NULL )
            free( types );
#endif
    }
    else {
        curr_hash.value = CollChk_basic_value(type);
        curr_hash.count = CollChk_basic_count(type);
    }

    type_hash->value = curr_hash.value;
    type_hash->count = curr_hash.count;
    for ( ii=1; ii < cnt; ii++ ) {
        CollChk_hash_add(type_hash, &curr_hash, type_hash);
    }
}

/*
    A wrapper that calls PMPI_Allreduce() provides different send and receive
    buffers so use of MPI_Allreduce() conforms to MPI-1 standard, section 2.2.
*/
int CollChk_Allreduce_int( int ival, MPI_Op op, MPI_Comm comm );
int CollChk_Allreduce_int( int ival, MPI_Op op, MPI_Comm comm )
{
    int  local_ival;
    PMPI_Allreduce( &ival, &local_ival, 1, MPI_INT, op, comm );
    return local_ival;
}

/*
   Checking if (type,cnt) is the same in all processes within the communicator.
*/
int CollChk_dtype_bcast(MPI_Comm comm, MPI_Datatype type, int cnt, int root,
                        char* call)
{
#if 0
    CollChk_hash_t  local_hash;        /* local hash value */
    CollChk_hash_t  root_hash;         /* root's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size;        /* rank, size */
    int             are_hashes_equal;  /* go flag, ok flag */

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* get the hash values */
    CollChk_dtype_hash(type, cnt, &local_hash);

    if (rank == root) {
        root_hash.value = local_hash.value;
        root_hash.count = local_hash.count;
    }
    /* broadcast root's datatype hash to all other processes */
    PMPI_Bcast(&root_hash, 2, MPI_UNSIGNED, root, comm);

    /* Compare root's datatype hash to the local hash */
    are_hashes_equal = CollChk_hash_equal( &local_hash, &root_hash );
    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    are_hashes_equal = CollChk_Allreduce_int(are_hashes_equal, MPI_LAND, comm);

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
#endif
#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_bcast()\n" );
#endif
    return CollChk_dtype_scatter(comm, type, cnt, type, cnt, root, 1, call );
}


/*
  The (sendtype,sendcnt) is assumed to be known in root process.
  (recvtype,recvcnt) is known in every process.  The routine checks if
  (recvtype,recvcnt) on each process is the same as (sendtype,sendcnt)
  on process root.
*/
int CollChk_dtype_scatter(MPI_Comm comm,
                          MPI_Datatype sendtype, int sendcnt,
                          MPI_Datatype recvtype, int recvcnt,
                          int root, int are2buffs, char *call)
{
    CollChk_hash_t  root_hash;         /* root's hash value */
    CollChk_hash_t  recv_hash;         /* local hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size;
    int             are_hashes_equal;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_scatter()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /*
       Scatter() only cares root's send datatype signature,
       i.e. ignore not-root's send datatype signatyre
    */
    /* Set the root's hash value */
    if (rank == root)
        CollChk_dtype_hash(sendtype, sendcnt, &root_hash);

    /* broadcast root's datatype hash to all other processes */
    PMPI_Bcast(&root_hash, 2, MPI_UNSIGNED, root, comm);

    /* Compare root_hash with the input/local hash */
    if ( are2buffs ) {
        CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );
        are_hashes_equal = CollChk_hash_equal( &root_hash, &recv_hash );
    }
    else
        are_hashes_equal = 1;

    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    are_hashes_equal = CollChk_Allreduce_int(are_hashes_equal, MPI_LAND, comm);

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}

/*
  The vector of (sendtype,sendcnts[]) is assumed to be known in root process.
  (recvtype,recvcnt) is known in every process.  The routine checks if
  (recvtype,recvcnt) on process P is the same as (sendtype,sendcnt[P])
  on process root. 
*/
int CollChk_dtype_scatterv(MPI_Comm comm,
                           MPI_Datatype sendtype, int *sendcnts,
                           MPI_Datatype recvtype, int recvcnt,
                           int root, int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;         /* hash array for (sendtype,sendcnts[]) */
    CollChk_hash_t  root_hash;       /* root's hash value */
    CollChk_hash_t  recv_hash;       /* local hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    int             rank, size, idx;
    int             are_hashes_equal;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_scatterv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /*
       Scatter() only cares root's send datatype signature[],
       i.e. ignore not-root's send datatype signatyre
    */
    hashes = NULL;
    if ( rank == root ) {
        /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
        hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
#else
        hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
#endif
        for ( idx = 0; idx < size; idx++ )
            CollChk_dtype_hash( sendtype, sendcnts[idx], &(hashes[idx]) );
    }

    /* Send the root's hash array to update other processes's root_hash */
    PMPI_Scatter(hashes, 2, MPI_UNSIGNED, &root_hash, 2, MPI_UNSIGNED,
                 root, comm);

    /* Compare root_hash with the input/local hash */
    if ( are2buffs ) {
        CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );    
        are_hashes_equal = CollChk_hash_equal( &root_hash, &recv_hash );
    }
    else
        are_hashes_equal = 1;

    if ( !are_hashes_equal )
        sprintf(err_str, "Inconsistent datatype signatures detected "
                         "between rank %d and rank %d.\n", rank, root);
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out if there is unequal hashes in the communicator */
    are_hashes_equal = CollChk_Allreduce_int(are_hashes_equal, MPI_LAND, comm);

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
#endif

    if ( !are_hashes_equal )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
   (sendtype,sendcnt) and (recvtype,recvcnt) are known in every process.
   The routine checks if (recvtype,recvcnt) on local process is the same
   as (sendtype,sendcnt) collected from all the other processes.
*/
int CollChk_dtype_allgather(MPI_Comm comm,
                            MPI_Datatype sendtype, int sendcnt,
                            MPI_Datatype recvtype, int recvcnt,
                            int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;      /* hashes from other senders' */
    CollChk_hash_t  send_hash;    /* local sender's hash value */
    CollChk_hash_t  recv_hash;    /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *isOK2chks;   /* boolean array, true:sendbuff=\=recvbuff */
    int             *err_ranks;   /* array of ranks that have mismatch hashes */
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_allgather()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    CollChk_dtype_hash( sendtype, sendcnt, &send_hash );
    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    hashes    = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) malloc( size * sizeof(int) );
    isOK2chks = (int *) malloc( size * sizeof(int) );
#else
    hashes    = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) alloca( size * sizeof(int) );
    isOK2chks = (int *) alloca( size * sizeof(int) );
#endif

    /* Gather other senders' datatype hashes as local hash arrary */
    PMPI_Allgather(&send_hash, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);
    PMPI_Allgather(&are2buffs, 1, MPI_INT, isOK2chks, 1, MPI_INT, comm);

    /* Compute the local datatype hash value */
    CollChk_dtype_hash( recvtype, recvcnt, &recv_hash );

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        if ( isOK2chks[idx] ) {
            if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
                err_ranks[ err_rank_size++ ] = idx;
        }
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    err_rank_size = CollChk_Allreduce_int(err_rank_size, MPI_SUM, comm);

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
    if ( isOK2chks != NULL )
        free( isOK2chks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtype,recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtype,recvcnts[]) on local process is the same as
  (sendtype,sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_allgatherv(MPI_Comm comm,
                             MPI_Datatype sendtype, int sendcnt,
                             MPI_Datatype recvtype, int *recvcnts,
                             int are2buffs, char *call)
{
    CollChk_hash_t  *hashes;      /* hash array for (sendtype,sendcnt) */
    CollChk_hash_t  send_hash;    /* local sender's hash value */
    CollChk_hash_t  recv_hash;    /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *isOK2chks;   /* boolean array, true:sendbuff=\=recvbuff */
    int             *err_ranks;   /* array of ranks that have mismatch hashes */
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_allgatherv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    hashes    = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) malloc( size * sizeof(int) );
    isOK2chks = (int *) malloc( size * sizeof(int) );
#else
    hashes    = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks = (int *) alloca( size * sizeof(int) );
    isOK2chks = (int *) alloca( size * sizeof(int) );
#endif

    CollChk_dtype_hash( sendtype, sendcnt, &send_hash );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Allgather(&send_hash, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);
    PMPI_Allgather(&are2buffs, 1, MPI_INT, isOK2chks, 1, MPI_INT, comm);

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        if ( isOK2chks[idx] ) {
            CollChk_dtype_hash( recvtype, recvcnts[idx], &recv_hash );
            if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
                err_ranks[ err_rank_size++ ] = idx;
        }
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    err_rank_size = CollChk_Allreduce_int(err_rank_size, MPI_SUM, comm);

#if ! defined( HAVE_ALLOCA )
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
    if ( isOK2chks != NULL )
        free( isOK2chks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtype,recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtype,recvcnts[]) on local process is the same as
  (sendtype,sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_alltoallv(MPI_Comm comm,
                            MPI_Datatype sendtype, int *sendcnts,
                            MPI_Datatype recvtype, int *recvcnts,
                            char *call)
{
    CollChk_hash_t  *send_hashes;    /* hash array for (sendtype,sendcnt[]) */
    CollChk_hash_t  *hashes;         /* hash array for (sendtype,sendcnt[]) */
    CollChk_hash_t  recv_hash;       /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *err_ranks;
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

#if defined( DEBUG )
    fprintf( stdout, "CollChk_dtype_alltoallv()\n" );
#endif

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    send_hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) malloc( size * sizeof(int) );
#else
    send_hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) alloca( size * sizeof(int) );
#endif

    for ( idx = 0; idx < size; idx++ )
        CollChk_dtype_hash( sendtype, sendcnts[idx], &send_hashes[idx] );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Alltoall(send_hashes, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);

    /* Compare the local datatype hash with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        CollChk_dtype_hash( recvtype, recvcnts[idx], &recv_hash );
        if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
            err_ranks[ err_rank_size++ ] = idx;
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    err_rank_size = CollChk_Allreduce_int(err_rank_size, MPI_SUM, comm);

#if ! defined( HAVE_ALLOCA )
    if ( send_hashes != NULL )
        free( send_hashes );
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}


/*
  The vector of (recvtypes[],recvcnts[]) is assumed to be known locally.
  The routine checks if (recvtypes[],recvcnts[]) on local process is the same as
  (sendtype[],sendcnts[]) collected from all the other processes.
*/
int CollChk_dtype_alltoallw(MPI_Comm comm,
                            MPI_Datatype *sendtypes, int *sendcnts,
                            MPI_Datatype *recvtypes, int *recvcnts,
                            char *call)
{
    CollChk_hash_t  *send_hashes;  /* hash array for (sendtypes[],sendcnt[]) */
    CollChk_hash_t  *hashes;       /* hash array for (sendtypes[],sendcnt[]) */
    CollChk_hash_t  recv_hash;     /* local receiver's hash value */
    char            err_str[COLLCHK_STD_STRLEN];
    char            rank_str[COLLCHK_SM_STRLEN];
    int             *err_ranks;
    int             err_rank_size;
    int             err_str_sz, str_sz;
    int             rank, size, idx;

    /* get the rank and size */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Allocate hash buffer memory */
#if ! defined( HAVE_ALLOCA )
    send_hashes = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) malloc( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) malloc( size * sizeof(int) );
#else
    send_hashes = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    hashes      = (CollChk_hash_t *) alloca( size * sizeof(CollChk_hash_t) );
    err_ranks   = (int *) alloca( size * sizeof(int) );
#endif

    for ( idx = 0; idx < size; idx++ )
        CollChk_dtype_hash( sendtypes[idx], sendcnts[idx], &send_hashes[idx] );

    /* Gather other senders' datatype hashes as local hash array */
    PMPI_Alltoall(send_hashes, 2, MPI_UNSIGNED, hashes, 2, MPI_UNSIGNED, comm);

    /* Compare the local datatype hashes with other senders' datatype hashes */
    /*
       The checks are more exhaustive and redundant tests on all processes,
       but matches what user expects
    */
    err_rank_size = 0;
    for ( idx = 0; idx < size; idx++ ) {
        CollChk_dtype_hash( recvtypes[idx], recvcnts[idx], &recv_hash );
        if ( ! CollChk_hash_equal( &recv_hash, &(hashes[idx]) ) )
            err_ranks[ err_rank_size++ ] = idx;
    }

    if ( err_rank_size > 0 ) {
        str_sz = sprintf(err_str, "Inconsistent datatype signatures detected "
                                  "between local rank %d and remote ranks,",
                                  rank);
        /* all string size variables, *_sz, does not include NULL character */
        err_str_sz = str_sz;
        for ( idx = 0; idx < err_rank_size; idx++ ) {
            str_sz = sprintf(rank_str, " %d", err_ranks[idx] );
            /* -3 is reserved for "..." */
            if ( str_sz + err_str_sz < COLLCHK_STD_STRLEN-3 ) {
                strcat(err_str, rank_str);
                err_str_sz = strlen( err_str );
            }
            else {
                strcat(err_str, "..." );
                break;
            }
        }
    }
    else
        sprintf(err_str, COLLCHK_NO_ERROR_STR);

    /* Find out the total number of unequal hashes in the communicator */
    err_rank_size = CollChk_Allreduce_int(err_rank_size, MPI_SUM, comm);

#if ! defined( HAVE_ALLOCA )
    if ( send_hashes != NULL )
        free( send_hashes );
    if ( hashes != NULL )
        free( hashes );
    if ( err_ranks != NULL )
        free( err_ranks );
#endif

    if ( err_rank_size > 0 )
        return CollChk_err_han(err_str, COLLCHK_ERR_DTYPE, call, comm);

    return MPI_SUCCESS;
}

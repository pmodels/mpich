/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

#include "ad_daos.h"

extern daos_handle_t daos_pool_oh;

#  define UINT64ENCODE(p, n) {						      \
   uint64_t _n = (n);							      \
   size_t _i;								      \
   uint8_t *_p = (uint8_t*)(p);						      \
									      \
   for (_i = 0; _i < sizeof(uint64_t); _i++, _n >>= 8)			      \
      *_p++ = (uint8_t)(_n & 0xff);					      \
   for (/*void*/; _i < 8; _i++)						      \
      *_p++ = 0;							      \
   (p) = (uint8_t*)(p) + 8;						      \
}

#  define UINT64DECODE(p, n) {						      \
   /* WE DON'T CHECK FOR OVERFLOW! */					      \
   size_t _i;								      \
									      \
   n = 0;								      \
   (p) += 8;								      \
   for (_i = 0; _i < sizeof(uint64_t); _i++)				      \
      n = (n << 8) | *(--p);						      \
   (p) += 8;								      \
}

/* Decode a variable-sized buffer */
/* (Assumes that the high bits of the integer will be zero) */
#  define DECODE_VAR(p, n, l) {						      \
   size_t _i;								      \
									      \
   n = 0;								      \
   (p) += l;								      \
   for (_i = 0; _i < l; _i++)						      \
      n = (n << 8) | *(--p);						      \
   (p) += l;								      \
}

/* Decode a variable-sized buffer into a 64-bit unsigned integer */
/* (Assumes that the high bits of the integer will be zero) */
#  define UINT64DECODE_VAR(p, n, l)     DECODE_VAR(p, n, l)

/* Multiply two 128 bit unsigned integers to yield a 128 bit unsigned integer */
static void
duuid_mult128(uint64_t x_lo, uint64_t x_hi, uint64_t y_lo, uint64_t y_hi,
    uint64_t *ans_lo, uint64_t *ans_hi)
{
    uint64_t xlyl;
    uint64_t xlyh;
    uint64_t xhyl;
    uint64_t xhyh;
    uint64_t temp;

    /*
     * First calculate x_lo * y_lo
     */
    /* Compute 64 bit results of multiplication of each combination of high and
     * low 32 bit sections of x_lo and y_lo */
    xlyl = (x_lo & 0xffffffff) * (y_lo & 0xffffffff);
    xlyh = (x_lo & 0xffffffff) * (y_lo >> 32);
    xhyl = (x_lo >> 32) * (y_lo & 0xffffffff);
    xhyh = (x_lo >> 32) * (y_lo >> 32);

    /* Calculate lower 32 bits of the answer */
    *ans_lo = xlyl & 0xffffffff;

    /* Calculate second 32 bits of the answer. Use temp to keep a 64 bit result
     * of the calculation for these 32 bits, to keep track of overflow past
     * these 32 bits. */
    temp = (xlyl >> 32) + (xlyh & 0xffffffff) + (xhyl & 0xffffffff);
    *ans_lo += temp << 32;

    /* Calculate third 32 bits of the answer, including overflowed result from
     * the previous operation */
    temp >>= 32;
    temp += (xlyh >> 32) + (xhyl >> 32) + (xhyh & 0xffffffff);
    *ans_hi = temp & 0xffffffff;

    /* Calculate highest 32 bits of the answer. No need to keep track of
     * overflow because it has overflowed past the end of the 128 bit answer */
    temp >>= 32;
    temp += (xhyh >> 32);
    *ans_hi += temp << 32;

    /*
     * Now add the results from multiplying x_lo * y_hi and x_hi * y_lo. No need
     * to consider overflow here, and no need to consider x_hi * y_hi because
     * those results would overflow past the end of the 128 bit answer.
     */
    *ans_hi += (x_lo * y_hi) + (x_hi * y_lo);

    return;
} /* end duuid_mult128() */

/* Implementation of the FNV hash algorithm */
static void
duuid_hash128(const char *name, void *hash)
{
    const uint8_t *name_p = (const uint8_t *)name;
    uint8_t *hash_p = (uint8_t *)hash;
    uint64_t name_lo;
    uint64_t name_hi;
    /* Initialize hash value in accordance with the FNV algorithm */
    uint64_t hash_lo = 0x62b821756295c58d;
    uint64_t hash_hi = 0x6c62272e07bb0142;
    /* Initialize FNV prime number in accordance with the FNV algorithm */
    const uint64_t fnv_prime_lo = 0x13b;
    const uint64_t fnv_prime_hi = 0x1000000;
    size_t name_len_rem = strlen(name);

    while(name_len_rem > 0) {
        /* "Decode" lower 64 bits of this 128 bit section of the name, so the
         * numberical value of the integer is the same on both little endian and
         * big endian systems */
        if(name_len_rem >= 8) {
            UINT64DECODE(name_p, name_lo)
            name_len_rem -= 8;
        } /* end if */
        else {
            name_lo = 0;
            UINT64DECODE_VAR(name_p, name_lo, name_len_rem)
            name_len_rem = 0;
        } /* end else */

        /* "Decode" second 64 bits */
        if(name_len_rem > 0) {
            if(name_len_rem >= 8) {
                UINT64DECODE(name_p, name_hi)
                name_len_rem -= 8;
            } /* end if */
            else {
                name_hi = 0;
                UINT64DECODE_VAR(name_p, name_hi, name_len_rem)
                name_len_rem = 0;
            } /* end else */
        } /* end if */
        else
            name_hi = 0;

        /* FNV algorithm - XOR hash with name then multiply by fnv_prime */
        hash_lo ^= name_lo;
        hash_hi ^= name_hi;
        duuid_mult128(hash_lo, hash_hi, fnv_prime_lo, fnv_prime_hi, &hash_lo, &hash_hi);
    } /* end while */

    /* "Encode" hash integers to char buffer, so the buffer is the same on both
     * little endian and big endian systems */
    UINT64ENCODE(hash_p, hash_lo)
    UINT64ENCODE(hash_p, hash_hi)

    return;
} /* end duuid_hash128() */

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code)
{
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_OPEN";
    int rc;

    *error_code = MPI_SUCCESS;

    /* check & Fail if container exists */
    if (fd->access_mode & ADIO_EXCL) {
        rc = daos_cont_open(daos_pool_oh, cont->uuid, DAOS_COO_RO, &cont->coh,
                            NULL, NULL);
        if (rc == 0) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "File Exists", 0);
            goto err_cont;
        }
    }

    /* Create DAOS container */
    if (fd->access_mode & ADIO_CREATE) {
        rc = daos_cont_create(daos_pool_oh, cont->uuid, NULL);
        if (rc != 0) {
            printf("failed to create container (%d)\n", rc);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Container Create Failed", 0);
            goto out;
        }
    }

    /* Open DAOS Container */
    rc = daos_cont_open(daos_pool_oh, cont->uuid, cont->amode, &cont->coh,
                        NULL, NULL);
    if (rc != 0) {
        printf("failed to open container (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Container Open Failed", 0);
        goto out;
    }

    if (cont->amode == DAOS_COO_RW) {
        rc = daos_epoch_hold(cont->coh, &cont->epoch, NULL, NULL);
        if (rc != 0) {
            printf("daos_epoch_hold failed (%d)\n", rc);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Epoch Hold Failed", 0);
            goto err_cont;
        }
    }
    else {
        daos_epoch_state_t state;

        rc = daos_epoch_query(cont->coh, &state, NULL);
        if (rc != 0) {
            printf("daos_epoch_query failed (%d)\n", rc);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Epoch Hold Failed", 0);
            goto err_cont;
        }
        cont->epoch = state.es_hce;
    }

    /* open array if not create mode */
    if (!(fd->access_mode & ADIO_CREATE)) {
        daos_size_t elem_size;

        rc = daos_array_open(cont->coh, cont->oid, cont->epoch, DAOS_OO_RW,
                             &elem_size, &cont->stripe_size, &cont->oh, NULL);
        if (rc != 0) {
            printf("daos_array_open() failed (%d)\n", rc);
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Array Create Failed", 0);
            goto err_cont;
        }
        return;
    }

    /* Create a DAOS byte array for the file */
    rc = daos_array_create(cont->coh, cont->oid, cont->epoch, 1,
                           cont->stripe_size, &cont->oh, NULL);
    if (rc != 0) {
        printf("daos_array_create() failed (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Array Create Failed", 0);
        goto err_cont;
    }

out:
    return;
err_cont:
    daos_cont_close(cont->coh, NULL);
    goto out;
}

void ADIOI_DAOS_OpenColl(ADIO_File fd, int rank,
	int access_mode, int *error_code)
{
    struct ADIO_DAOS_cont *cont;
    int amode;
    MPI_Comm comm = fd->comm;
    int mpi_size;
    int rc;
    static char myname[] = "ADIOI_DAOS_OPENCOLL";

    MPI_Comm_size(comm, &mpi_size);

    if (access_mode & ADIO_WRONLY) {
	*error_code = ADIOI_Err_create_code(myname, fd->filename, errno);
        return;
    }

    amode = 0;
    if (access_mode & ADIO_RDONLY)
	amode = DAOS_COO_RO;        
    else
        amode = DAOS_COO_RW;

    cont = (struct ADIO_DAOS_cont *)ADIOI_Malloc(sizeof(struct ADIO_DAOS_cont));
    if (cont == NULL) {
        printf("mem alloc failed.\n");
	*error_code = MPIO_Err_create_code(MPI_SUCCESS,
					   MPIR_ERR_RECOVERABLE,
					   myname, __LINE__,
					   MPI_ERR_UNKNOWN,
					   "Error allocating memory", 0);
	return;
    }

#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_open_a, 0, NULL );
#endif
    fd->access_mode = access_mode;

    cont->amode = amode;
    /* Hash file name to create uuid */
    duuid_hash128(fd->filename, &cont->uuid);

    /* MSC - hardcode to 1MB for now, check for info hint later. */
    cont->stripe_size = 1048576;

    /* MSC - make OID = 0 for now; hash file name to oid later. */
    cont->oid.lo = 0;
    cont->oid.mid = 0;
    cont->oid.hi = 0;
    daos_obj_id_generate(&cont->oid, DAOS_OC_REPL_MAX_RW);

    /* MSC - set to 0 for now.. do hold later */
    cont->epoch = 0;

    fd->fs_ptr = cont;
    if (rank == 0)
        (*(fd->fns->ADIOI_xxx_Open))(fd, error_code);

    if (mpi_size > 1)
        MPI_Bcast(error_code, 1, MPI_INT, 0, comm);

    if (*error_code != MPI_SUCCESS)
        goto err_free;

    if (mpi_size > 1)
        handle_share(&cont->coh, HANDLE_CO, rank, daos_pool_oh, comm);

    MPI_Bcast(&cont->epoch, 1, MPI_UINT64_T, 0, comm);

    /* open array on other ranks */
    if (rank != 0) {
        daos_size_t elem_size;
        daos_size_t stripe_size;

        rc = daos_array_open(cont->coh, cont->oid, cont->epoch, DAOS_OO_RW,
                             &elem_size, &stripe_size, &cont->oh, NULL);
        if (rc != 0) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               ADIOI_DAOS_error_convert(rc),
                                               "Array Open Failed", 0);
            goto err_free;
        }
        if (elem_size != 1 || stripe_size != cont->stripe_size) {
            *error_code = MPI_UNDEFINED;
            goto err_free;
        }
    }
    MPI_Barrier(comm);
    fd->is_open = 1;

#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_open_b, 0, NULL );
#endif

    return;

err_free:
    free(cont);
    return;
}

void ADIOI_DAOS_Delete(const char *filename, int *error_code)
{
    int ret;
    uuid_t uuid;
    static char myname[] = "ADIOI_DAOS_DELETE";

    /* Hash file name to container uuid */
    duuid_hash128(filename, &uuid);

    ret = daos_cont_destroy(daos_pool_oh, uuid, 1, NULL);
    if (ret != 0) {
        printf("failed to destroy container (%d)\n", ret);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(ret),
                                           "Container Create Failed", 0);
        return;
    }

    *error_code = MPI_SUCCESS;
    return;
}

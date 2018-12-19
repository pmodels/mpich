/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 * Copyright (C) 1997 University of Chicago.
 * See COPYRIGHT notice in top-level directory.
 *
 * Copyright (C) 2018 Intel Corporation
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform, display,
 * or disclose this software are subject to the terms of the Apache License as
 * provided in Contract No. 8F-30005.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 */

#include <libgen.h>
#include "ad_daos.h"
#include <uuid/uuid.h>

#define OID_SEED 5731

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
duuid_hash128(const char *name, void *hash, uint64_t *hi, uint64_t *lo)
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

    if (hi)
        *hi = hash_hi;
    if (lo)
        *lo = hash_lo;

    return;
} /* end duuid_hash128() */

static void
parse_filename(const char *path, char **_obj_name, char **_cont_name)
{
    char *f1 = strdup(path);
    char *f2 = strdup(path);
    char *fname = basename(f1);
    char *cont_name = dirname(f2);
    
    if (cont_name[0] == '.' || cont_name[0] != '/') {
        char cwd[1024];

        getcwd(cwd, 1024);

        if (strcmp(cont_name, ".") == 0) {
            cont_name = strdup(cwd);
        }
        else {
            char *new_dir = calloc(strlen(cwd) + strlen(cont_name) + 1,
                                   sizeof(char));

            strcpy(new_dir, cwd);
            if (cont_name[0] == '.')
                strcat(new_dir, &cont_name[1]);
            else {
                strcat(new_dir, "/");
                strcat(new_dir, cont_name);
            }
            cont_name = new_dir;
        }
        *_cont_name = cont_name;
    }
    else {
        *_cont_name = strdup(cont_name);
    }
   
    *_obj_name = strdup(fname);

    free(f1);
    free(f2);
}

void
ADIOI_DAOS_Open(ADIO_File fd, int *error_code)
{
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_OPEN";
    int perm, old_mask, amode;
    int rc;

    *error_code = MPI_SUCCESS;

    rc = adio_daos_cont_lookup_create(cont->uuid,
                                      (fd->access_mode & ADIO_CREATE),
                                      &cont->hdl);
    if (rc) {
        PRINT_MSG(stderr, "container lookup failed (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "Failed to create container", 0);
        goto out;
    }

    cont->coh = cont->hdl->co_coh;
    if (cont->amode == DAOS_COO_RW)
        amode = O_RDWR;
    else
        amode = O_RDONLY;

    /* Mount a DFS namespace on the container */
    rc = dfs_mount(daos_pool_oh, cont->coh, amode, &cont->dfs);
    if (rc) {
        PRINT_MSG(stderr, "Failed to mount flat namespace (%d)\n", rc);
        goto err_cont;
    }

    /* Set file access flags */
    amode = 0;
    if (fd->access_mode & ADIO_CREATE)
        amode = amode | O_CREAT;
    if (fd->access_mode & ADIO_RDONLY)
        amode = amode | O_RDONLY;
    if (fd->access_mode & ADIO_WRONLY)
        amode = amode | O_WRONLY;
    if (fd->access_mode & ADIO_RDWR)
        amode = amode | O_RDWR;
    if (fd->access_mode & ADIO_EXCL)
        amode = amode | O_EXCL;

    /* Set DFS permission mode + object type */
    if (fd->perm == ADIO_PERM_NULL) {
        old_mask = umask(022);
        umask(old_mask);
        perm = old_mask ^ 0666;
    } else {
        perm = fd->perm;
    }
    perm = S_IFREG | perm;

    rc = dfs_open(cont->dfs, NULL, cont->obj_name, perm, amode,
                  fd->hints->fs_hints.daos.obj_class, NULL, &cont->obj);
    if (rc) {
        PRINT_MSG(stderr, "Failed to open file %s (%d)\n", cont->obj_name, rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "dfs_open() failed", 0);
        goto err_dfs;
    }

    rc = dfs_get_file_oh(cont->obj, &cont->oh);
    if (rc) {
        PRINT_MSG(stderr, "Failed to get DFS object handle (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "dfs_get_file_oh() Failed", 0);
        goto err_obj;
    }

out:
    return;
err_obj:
    dfs_release(cont->obj);
    if (fd->access_mode & ADIO_CREATE)
        dfs_remove(cont->dfs, NULL, cont->obj_name, true);
err_dfs:
    dfs_umount(cont->dfs);
err_cont:
    adio_daos_cont_release(cont->hdl);
    goto out;
}

void
ADIOI_DAOS_OpenColl(ADIO_File fd, int rank, int access_mode, int *error_code)
{
    struct ADIO_DAOS_cont *cont;
    int amode;
    MPI_Comm comm = fd->comm;
    int mpi_size;
    int rc;
    static char myname[] = "ADIOI_DAOS_OPENCOLL";

    MPI_Comm_size(comm, &mpi_size);

    amode = 0;
    if (access_mode & ADIO_RDONLY)
	amode = DAOS_COO_RO;        
    else
        amode = DAOS_COO_RW;

    cont = (struct ADIO_DAOS_cont *)ADIOI_Malloc(sizeof(struct ADIO_DAOS_cont));
    if (cont == NULL) {
        PRINT_MSG(stderr, "mem alloc failed.\n");
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

    parse_filename(fd->filename, &cont->obj_name, &cont->cont_name);

    /* Hash container name to create uuid */
    duuid_hash128(cont->cont_name, &cont->uuid, NULL, NULL);

    cont->chunk_size = fd->hints->fs_hints.daos.chunk_size;
    cont->hdl = NULL;
    fd->fs_ptr = cont;

#if 0
    {
        char uuid_str[37];
        uuid_unparse(cont->uuid, uuid_str);

        fprintf(stderr, "Container Open %s %s\n", cont->cont_name, uuid_str);
        fprintf(stderr, "File %s\n", cont->obj_name);
    }
    fprintf(stderr, "chunk_size  = %d\n", fd->hints->fs_hints.daos.chunk_size);
    fprintf(stderr, "OCLASS  = %d\n", fd->hints->fs_hints.daos.obj_class);
#endif

    if (rank == 0) {
        (*(fd->fns->ADIOI_xxx_Open))(fd, error_code);
        MPI_Error_class(*error_code, &rc);
    }

    if (mpi_size > 1) {
        MPI_Bcast(&rc, 1, MPI_INT, 0, comm);

        if (rank != 0) {
            if (rc)
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                   MPIR_ERR_RECOVERABLE,
                                                   myname, __LINE__, rc,
                                                   "File Open Failed", 0);
            else
                *error_code = MPI_SUCCESS;
        }
    }
    if (*error_code != MPI_SUCCESS)
        goto err_free;

    if (mpi_size > 1) {
        handle_share(&cont->coh, HANDLE_CO, rank, daos_pool_oh, comm);
        handle_share(&cont->oh, HANDLE_OBJ, rank, cont->coh, comm);
    }

    fd->is_open = 1;

#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event( ADIOI_MPE_open_b, 0, NULL );
#endif

    return;

err_free:
    if (cont->obj_name)
        free(cont->obj_name);
    if (cont->cont_name)
        free(cont->cont_name);
    ADIOI_Free(cont);
    return;
}

void
ADIOI_DAOS_Flush(ADIO_File fd, int *error_code)
{
    int rank, rc, err;
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_FLUSH";

    MPI_Comm_rank(fd->comm, &rank);

    if (fd->is_open <= 0)
        goto out;

    daos_sync_ranks(fd->comm);

    if (rank == 0) {
        rc = dfs_sync(cont->dfs);
        if (rc) {
            PRINT_MSG(stderr, "dfs_sync() failed (%d)\n", rc);
            goto bcast_rc;
        }
    }

bcast_rc:
    MPI_Bcast(&rc, 1, MPI_INT, 0, fd->comm);

    if (rc != 0) {
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "DFS Sync Failed", 0);
        return;
    }
out:
    *error_code = MPI_SUCCESS;
}

void
ADIOI_DAOS_Delete(const char *filename, int *error_code)
{
    struct adio_daos_co_hdl *co_hdl;
    uuid_t uuid;
    dfs_t *dfs;
    char *obj_name, *cont_name;
    static char myname[] = "ADIOI_DAOS_DELETE";
    int rc;

    parse_filename(filename, &obj_name, &cont_name);

    /* Hash container name to container uuid */
    duuid_hash128(cont_name, &uuid, NULL, NULL);

    rc = adio_daos_cont_lookup_create(uuid, false, &co_hdl);
    if (rc || co_hdl == NULL) {
        *error_code = MPI_SUCCESS;
        goto out_free;
    }

    /* Mount a flat namespace on the container */
    rc = dfs_mount(daos_pool_oh, co_hdl->co_coh, O_RDWR, &dfs);
    if (rc) {
        PRINT_MSG(stderr, "Failed to mount flat namespace (%d)\n", rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "dfs_mount() Failed", 0);
        goto out_cont;
    }

    /* Remove the file from the flat namespace */
    rc = dfs_remove(dfs, NULL, obj_name, true);
    if (rc) {
        PRINT_MSG(stderr, "Failed to delete file %s (%d)\n", obj_name, rc);
        *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                           MPIR_ERR_RECOVERABLE,
                                           myname, __LINE__,
                                           ADIOI_DAOS_error_convert(rc),
                                           "dfs_remove() Failed", 0);
        goto out_dfs;
    }
    *error_code = MPI_SUCCESS;

out_dfs:
    dfs_umount(dfs);
out_cont:
    adio_daos_cont_release(co_hdl);
out_free:
    free(obj_name);
    free(cont_name);
    return;
}

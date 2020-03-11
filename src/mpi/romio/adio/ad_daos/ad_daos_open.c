/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2020 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2018-2020 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "ad_daos.h"
#include <libgen.h>
#include <uuid/uuid.h>
#include <gurt/common.h>
#include <daos_uns.h>

#define OID_SEED 5731

#define UINT64ENCODE(p, n) {						      \
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

#define UINT64DECODE(p, n) {						      \
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
#define DECODE_VAR(p, n, l) {						      \
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
#define UINT64DECODE_VAR(p, n, l)     DECODE_VAR(p, n, l)

/* Multiply two 128 bit unsigned integers to yield a 128 bit unsigned integer */
static void
duuid_mult128(uint64_t x_lo, uint64_t x_hi, uint64_t y_lo, uint64_t y_hi,
              uint64_t * ans_lo, uint64_t * ans_hi)
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
}       /* end duuid_mult128() */

/* Implementation of the FNV hash algorithm */
static void duuid_hash128(const char *name, void *hash, uint64_t * hi, uint64_t * lo)
{
    const uint8_t *name_p = (const uint8_t *) name;
    uint8_t *hash_p = (uint8_t *) hash;
    uint64_t name_lo;
    uint64_t name_hi;
    /* Initialize hash value in accordance with the FNV algorithm */
    uint64_t hash_lo = 0x62b821756295c58d;
    uint64_t hash_hi = 0x6c62272e07bb0142;
    /* Initialize FNV prime number in accordance with the FNV algorithm */
    const uint64_t fnv_prime_lo = 0x13b;
    const uint64_t fnv_prime_hi = 0x1000000;
    size_t name_len_rem = strlen(name);

    while (name_len_rem > 0) {
        /* "Decode" lower 64 bits of this 128 bit section of the name, so the
         * numberical value of the integer is the same on both little endian and
         * big endian systems */
        if (name_len_rem >= 8) {
            UINT64DECODE(name_p, name_lo)
                name_len_rem -= 8;
        } /* end if */
        else {
            name_lo = 0;
            UINT64DECODE_VAR(name_p, name_lo, name_len_rem)
                name_len_rem = 0;
        }       /* end else */

        /* "Decode" second 64 bits */
        if (name_len_rem > 0) {
            if (name_len_rem >= 8) {
                UINT64DECODE(name_p, name_hi)
                    name_len_rem -= 8;
            } /* end if */
            else {
                name_hi = 0;
                UINT64DECODE_VAR(name_p, name_hi, name_len_rem)
                    name_len_rem = 0;
            }   /* end else */
        } /* end if */
        else
            name_hi = 0;

        /* FNV algorithm - XOR hash with name then multiply by fnv_prime */
        hash_lo ^= name_lo;
        hash_hi ^= name_hi;
        duuid_mult128(hash_lo, hash_hi, fnv_prime_lo, fnv_prime_hi, &hash_lo, &hash_hi);
    }   /* end while */

    /* "Encode" hash integers to char buffer, so the buffer is the same on both
     * little endian and big endian systems */
    UINT64ENCODE(hash_p, hash_lo)
        UINT64ENCODE(hash_p, hash_hi)

        if (hi)
        *hi = hash_hi;
    if (lo)
        *lo = hash_lo;

    return;
}       /* end duuid_hash128() */

static int parse_filename(const char *path, char **_obj_name, char **_cont_name)
{
    char *f1;
    char *f2;
    char *fname;
    char *cont_name;
    int rc = 0;

    f1 = ADIOI_Strdup(path);
    if (f1 == NULL)
        return ENOMEM;

    f2 = ADIOI_Strdup(path);
    if (f2 == NULL) {
        ADIOI_Free(f1);
        return ENOMEM;
    }

    fname = basename(f1);
    cont_name = dirname(f2);

    if (cont_name[0] == '.' || cont_name[0] != '/') {
        char *ptr;
        char cwd[PATH_MAX];

        ptr = getcwd(cwd, PATH_MAX);
        if (ptr == NULL) {
            rc = errno;
            goto out;
        }

        if (strcmp(cont_name, ".") == 0) {
            cont_name = ADIOI_Strdup(cwd);
            if (cont_name == NULL) {
                rc = ENOMEM;
                goto out;
            }
        } else {
            char *new_dir = ADIOI_Calloc(strlen(cwd) + strlen(cont_name) + 1,
                                         sizeof(char));

            if (new_dir == NULL) {
                rc = ENOMEM;
                goto out;
            }

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
    } else {
        *_cont_name = ADIOI_Strdup(cont_name);
        if (*_cont_name == NULL) {
            rc = ENOMEM;
            goto out;
        }
    }

    *_obj_name = ADIOI_Strdup(fname);
    if (*_obj_name == NULL) {
        rc = ENOMEM;
        goto out;
    }

  out:
    ADIOI_Free(f1);
    ADIOI_Free(f2);
    return rc;
}

static int cache_handles(struct ADIO_DAOS_cont *cont)
{
    char *uuid_str;
    static char myname[] = "ADIOI_DAOS_OPEN";
    int rc;

    cont->c = adio_daos_coh_lookup(cont->cuuid);
    if (cont->c == NULL) {
        /** insert handle into container hashtable */
        rc = adio_daos_coh_insert(cont->cuuid, cont->coh, &cont->c);
    } else {
        /** g2l handle not needed, already cached */
        daos_cont_close(cont->coh, NULL);
        cont->coh = cont->c->open_hdl;
    }
    if (rc)
        return rc;

    cont->p = adio_daos_poh_lookup(cont->puuid);
    if (cont->p == NULL) {
        /** insert handle into pool hashtable */
        rc = adio_daos_poh_insert(cont->puuid, cont->poh, &cont->p);
    } else {
        /** g2l handle not needed, already cached */
        rc = daos_pool_disconnect(cont->poh, NULL);
        cont->poh = cont->p->open_hdl;
    }

    return rc;
}

static int share_cont_info(struct ADIO_DAOS_cont *cont, int rank, MPI_Comm comm)
{
    char uuid_buf[74];
    d_iov_t pool_hdl = { NULL, 0, 0 };
    d_iov_t cont_hdl = { NULL, 0, 0 };
    d_iov_t dfs_hdl = { NULL, 0, 0 };
    d_iov_t file_hdl = { NULL, 0, 0 };
    char *buf = NULL;
    uint64_t total_size = 0;
    int rc = 0;

    if (rank == 0) {
        rc = daos_pool_local2global(cont->poh, &pool_hdl);
        if (rc)
            return rc;
        rc = daos_cont_local2global(cont->coh, &cont_hdl);
        if (rc)
            return rc;
        rc = dfs_local2global(cont->dfs, &dfs_hdl);
        if (rc)
            return rc;
        rc = dfs_obj_local2global(cont->dfs, cont->obj, &file_hdl);
        if (rc)
            return rc;

        total_size = sizeof(uuid_buf) + pool_hdl.iov_buf_len + cont_hdl.iov_buf_len +
            dfs_hdl.iov_buf_len + file_hdl.iov_buf_len + sizeof(daos_size_t) * 4;
    }

    /** broadcast size to all peers */
    rc = MPI_Bcast(&total_size, 1, MPI_UINT64_T, 0, comm);
    if (rc != MPI_SUCCESS)
        return -1;

    /** allocate buffers */
    buf = ADIOI_Malloc(total_size);
    if (buf == NULL)
        return -1;

    if (rank == 0) {
        char *ptr = buf;

        uuid_unparse(cont->puuid, ptr);
        ptr += 37;
        uuid_unparse(cont->cuuid, ptr);
        ptr += 37;

        *((daos_size_t *) ptr) = pool_hdl.iov_buf_len;
        ptr += sizeof(daos_size_t);
        pool_hdl.iov_buf = ptr;
        pool_hdl.iov_len = pool_hdl.iov_buf_len;
        rc = daos_pool_local2global(cont->poh, &pool_hdl);
        if (rc)
            goto out;
        ptr += pool_hdl.iov_buf_len;

        *((daos_size_t *) ptr) = cont_hdl.iov_buf_len;
        ptr += sizeof(daos_size_t);
        cont_hdl.iov_buf = ptr;
        cont_hdl.iov_len = cont_hdl.iov_buf_len;
        rc = daos_cont_local2global(cont->coh, &cont_hdl);
        if (rc)
            goto out;
        ptr += cont_hdl.iov_buf_len;

        *((daos_size_t *) ptr) = dfs_hdl.iov_buf_len;
        ptr += sizeof(daos_size_t);
        dfs_hdl.iov_buf = ptr;
        dfs_hdl.iov_len = dfs_hdl.iov_buf_len;
        rc = dfs_local2global(cont->dfs, &dfs_hdl);
        if (rc)
            goto out;
        ptr += dfs_hdl.iov_buf_len;

        *((daos_size_t *) ptr) = file_hdl.iov_buf_len;
        ptr += sizeof(daos_size_t);
        file_hdl.iov_buf = ptr;
        file_hdl.iov_len = file_hdl.iov_buf_len;
        rc = dfs_obj_local2global(cont->dfs, cont->obj, &file_hdl);
        if (rc)
            goto out;
    }

    rc = MPI_Bcast(buf, total_size, MPI_BYTE, 0, comm);
    if (rc != MPI_SUCCESS)
        goto out;

    if (rank != 0) {
        char *ptr = buf;

        rc = uuid_parse(ptr, cont->puuid);
        if (rc)
            goto out;
        ptr += 37;

        rc = uuid_parse(ptr, cont->cuuid);
        if (rc)
            goto out;
        ptr += 37;

        pool_hdl.iov_buf_len = *((daos_size_t *) ptr);
        ptr += sizeof(daos_size_t);
        pool_hdl.iov_buf = ptr;
        pool_hdl.iov_len = pool_hdl.iov_buf_len;
        rc = daos_pool_global2local(pool_hdl, &cont->poh);
        if (rc)
            goto out;
        ptr += pool_hdl.iov_buf_len;

        cont_hdl.iov_buf_len = *((daos_size_t *) ptr);
        ptr += sizeof(daos_size_t);
        cont_hdl.iov_buf = ptr;
        cont_hdl.iov_len = cont_hdl.iov_buf_len;
        rc = daos_cont_global2local(cont->poh, cont_hdl, &cont->coh);
        if (rc)
            goto out;
        ptr += cont_hdl.iov_buf_len;

        rc = cache_handles(cont);
        if (rc)
            goto out;

        dfs_hdl.iov_buf_len = *((daos_size_t *) ptr);
        ptr += sizeof(daos_size_t);
        dfs_hdl.iov_buf = ptr;
        dfs_hdl.iov_len = dfs_hdl.iov_buf_len;
        rc = dfs_global2local(cont->poh, cont->coh, O_RDWR, dfs_hdl, &cont->dfs);
        if (rc)
            goto out;
        ptr += dfs_hdl.iov_buf_len;

        if (rank != 0) {
            if (cont->c->dfs == NULL) {
                cont->c->dfs = cont->dfs;
            } else {
                dfs_umount(cont->dfs);
                cont->dfs = cont->c->dfs;
            }
        }

        file_hdl.iov_buf_len = *((daos_size_t *) ptr);
        ptr += sizeof(daos_size_t);
        file_hdl.iov_buf = ptr;
        file_hdl.iov_len = file_hdl.iov_buf_len;
        rc = dfs_obj_global2local(cont->dfs, 0, file_hdl, &cont->obj);
        if (rc)
            goto out;
    }

  out:
    ADIOI_Free(buf);
    return rc;
}

enum {
    HANDLE_POOL,
    HANDLE_CO,
    HANDLE_DFS,
    HANDLE_OBJ
};

static inline int
handle_share(daos_handle_t * poh, daos_handle_t * coh, dfs_t ** dfs,
             dfs_obj_t ** obj, int type, int rank, MPI_Comm comm)
{
    d_iov_t ghdl = { NULL, 0, 0 };
    int rc;

    if (rank == 0) {
        switch (type) {
            case HANDLE_POOL:
                rc = daos_pool_local2global(*poh, &ghdl);
                break;
            case HANDLE_CO:
                rc = daos_cont_local2global(*coh, &ghdl);
                break;
            case HANDLE_DFS:
                rc = dfs_local2global(*dfs, &ghdl);
                break;
            case HANDLE_OBJ:
                rc = dfs_obj_local2global(*dfs, *obj, &ghdl);
                break;
            default:
                assert(0);
        }
        if (rc)
            ghdl.iov_buf_len = 0;
    }

    /** broadcast size of global handle to all peers */
    rc = MPI_Bcast(&ghdl.iov_buf_len, 1, MPI_UINT64_T, 0, comm);
    if (rc != MPI_SUCCESS)
        return -1;

    if (ghdl.iov_buf_len == 0)
        return -1;

    /** allocate buffer for global handle */
    ghdl.iov_buf = ADIOI_Malloc(ghdl.iov_buf_len);
    ghdl.iov_len = ghdl.iov_buf_len;

    if (rank == 0) {
        /** generate actual global handle to share with peer tasks */
        switch (type) {
            case HANDLE_POOL:
                rc = daos_pool_local2global(*poh, &ghdl);
                break;
            case HANDLE_CO:
                rc = daos_cont_local2global(*coh, &ghdl);
                break;
            case HANDLE_DFS:
                rc = dfs_local2global(*dfs, &ghdl);
                break;
            case HANDLE_OBJ:
                rc = dfs_obj_local2global(*dfs, *obj, &ghdl);
                break;
            default:
                assert(0);
        }
    }

    /** broadcast global handle to all peers */
    rc = MPI_Bcast(ghdl.iov_buf, ghdl.iov_len, MPI_BYTE, 0, comm);
    if (rc != MPI_SUCCESS) {
        ADIOI_Free(ghdl.iov_buf);
        return -1;
    }

    if (rank != 0) {
        /** unpack global handle */
        switch (type) {
            case HANDLE_POOL:
                rc = daos_pool_global2local(ghdl, poh);
                break;
            case HANDLE_CO:
                rc = daos_cont_global2local(*poh, ghdl, coh);
                break;
            case HANDLE_DFS:
                rc = dfs_global2local(*poh, *coh, O_RDWR, ghdl, dfs);
                break;
            case HANDLE_OBJ:
                rc = dfs_obj_global2local(*dfs, 0, ghdl, obj);
                break;
            default:
                assert(0);
        }
    }

    ADIOI_Free(ghdl.iov_buf);
    return rc;
}

int
get_pool_cont_uuids(const char *path, uuid_t * puuid, uuid_t * cuuid,
                    daos_oclass_id_t * oclass, daos_size_t * chunk_size)
{
    bool use_duns = false;
    char *uuid_str;
    struct duns_attr_t attr;
    int rc;

    d_getenv_bool("DAOS_USE_DUNS", &use_duns);

    if (use_duns) {
        /* TODO: This works only for the DAOS pool/container info on the direct
         * parent dir. we still don't support nested dirs in the UNS. */
        rc = duns_resolve_path(path, &attr);
        if (rc) {
            PRINT_MSG(stderr, "duns_resolve_path() failed on path %s\n", path, rc);
            return -DER_INVAL;
        }

        if (attr.da_type != DAOS_PROP_CO_LAYOUT_POSIX) {
            PRINT_MSG(stderr, "Invalid DAOS container type\n");
            return -DER_INVAL;
        }

        uuid_copy(*puuid, attr.da_puuid);
        uuid_copy(*cuuid, attr.da_cuuid);
        *oclass = (attr.da_oclass_id == OC_UNKNOWN) ? OC_SX : attr.da_oclass_id;
        *chunk_size = attr.da_chunk_size;
    }

    /* use the env variables to retrieve the pool and container */
    uuid_str = getenv("DAOS_POOL");
    if (uuid_str == NULL) {
        PRINT_MSG(stderr, "Can't retrieve DAOS pool uuid\n");
        return -DER_INVAL;
    }
    if (uuid_parse(uuid_str, *puuid) < 0) {
        PRINT_MSG(stderr, "Failed to parse pool uuid\n");
        return -DER_INVAL;
    }

    uuid_str = getenv("DAOS_CONT");
    if (uuid_str == NULL) {
        /* TODO: remove this later and fail in this case. */
        /* Hash container name to create uuid */
        duuid_hash128(path, cuuid, NULL, NULL);
    } else {
        if (uuid_parse(uuid_str, *cuuid) < 0) {
            PRINT_MSG(stderr, "Failed to parse container uuid\n");
            return -DER_INVAL;
        }
    }

    *oclass = OC_UNKNOWN;
    *chunk_size = 0;

    return 0;
}

void ADIOI_DAOS_Open(ADIO_File fd, int *error_code)
{
    struct ADIO_DAOS_cont *cont = fd->fs_ptr;
    static char myname[] = "ADIOI_DAOS_OPEN";
    int perm, old_mask, amode;
    int rc;

    *error_code = MPI_SUCCESS;

    rc = get_pool_cont_uuids(cont->cont_name, &cont->puuid, &cont->cuuid,
                             &cont->obj_class, &cont->chunk_size);
    if (rc) {
        *error_code = ADIOI_DAOS_err(myname, cont->cont_name, __LINE__, rc);
        return;
    }

    /** Info object setting should override */
    if (fd->hints->fs_hints.daos.obj_class != OC_UNKNOWN)
        cont->obj_class = fd->hints->fs_hints.daos.obj_class;
    if (fd->hints->fs_hints.daos.chunk_size != 0)
        cont->chunk_size = fd->hints->fs_hints.daos.chunk_size;

#if 0
    {
        char uuid_str[37];
        uuid_unparse(cont->cuuid, uuid_str);

        fprintf(stderr, "Container Open %s %s\n", cont->cont_name, uuid_str);
        fprintf(stderr, "File %s\n", cont->obj_name);
    }
    fprintf(stderr, "chunk_size  = %d\n", cont->chunk_size);
    fprintf(stderr, "OCLASS  = %d\n", cont->obj_class);
#endif

    rc = adio_daos_poh_lookup_connect(cont->puuid, &cont->p);
    if (rc) {
        PRINT_MSG(stderr, "Failed to connect to DAOS Pool (%d)\n", rc);
        *error_code = ADIOI_DAOS_err(myname, cont->cont_name, __LINE__, rc);
        return;
    }

    cont->poh = cont->p->open_hdl;

    rc = adio_daos_coh_lookup_create(cont->poh, cont->cuuid, O_RDWR,
                                     (fd->access_mode & ADIO_CREATE), &cont->c);
    if (rc) {
        *error_code = ADIOI_DAOS_err(myname, cont->cont_name, __LINE__, rc);
        goto err_pool;
    }

    cont->coh = cont->c->open_hdl;

    assert(cont->c->dfs);
    cont->dfs = cont->c->dfs;

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
                  cont->obj_class, cont->chunk_size, NULL, &cont->obj);
    if (rc) {
        *error_code = ADIOI_DAOS_err(myname, cont->obj_name, __LINE__, rc);
        goto err_cont;
    }

  out:
    return;
  err_obj:
    dfs_release(cont->obj);
    if (fd->access_mode & ADIO_CREATE)
        dfs_remove(cont->dfs, NULL, cont->obj_name, true, NULL);
  err_cont:
    adio_daos_coh_release(cont->c);
    cont->c = NULL;
  err_pool:
    adio_daos_poh_release(cont->p);
    cont->p = NULL;
    goto out;
}

static int share_uuid_info(struct ADIO_DAOS_cont *cont, int rank, MPI_Comm comm)
{
    char buf[74];
    int rc;

    if (rank == 0) {
        uuid_unparse(cont->puuid, buf);
        uuid_unparse(cont->cuuid, buf + 37);
    }
    rc = MPI_Bcast(buf, sizeof(buf), MPI_BYTE, 0, comm);
    if (rc != MPI_SUCCESS)
        return rc;

    if (rank != 0) {
        rc = uuid_parse(buf, cont->puuid);
        if (rc)
            return rc;

        rc = uuid_parse(buf + 37, cont->cuuid);
        if (rc)
            return rc;
    }
    return 0;
}

void ADIOI_DAOS_OpenColl(ADIO_File fd, int rank, int access_mode, int *error_code)
{
    struct ADIO_DAOS_cont *cont;
    int amode, orig_amode_wronly;
    MPI_Comm comm = fd->comm;
    int mpi_size;
    int rc;
    static char myname[] = "ADIOI_DAOS_OPENCOLL";

    ADIOI_DAOS_Init(error_code);
    if (*error_code != MPI_SUCCESS)
        return;

    MPI_Comm_size(comm, &mpi_size);

    orig_amode_wronly = access_mode;
    if (access_mode & ADIO_WRONLY) {
        access_mode = access_mode ^ ADIO_WRONLY;
        access_mode = access_mode | ADIO_RDWR;
    }
    fd->access_mode = access_mode;

    amode = 0;
    if (access_mode & ADIO_RDONLY)
        amode = DAOS_COO_RO;
    else
        amode = DAOS_COO_RW;

    cont = (struct ADIO_DAOS_cont *) ADIOI_Malloc(sizeof(struct ADIO_DAOS_cont));
    if (cont == NULL) {
        *error_code = MPI_ERR_NO_MEM;
        return;
    }

    fd->access_mode = access_mode;
    cont->amode = amode;
    rc = parse_filename(fd->filename, &cont->obj_name, &cont->cont_name);
    if (rc) {
        *error_code = MPI_ERR_NO_MEM;
        return;
    }

    cont->p = NULL;
    cont->c = NULL;
    fd->fs_ptr = cont;

    if (rank == 0) {
        (*(fd->fns->ADIOI_xxx_Open)) (fd, error_code);
        MPI_Error_class(*error_code, &rc);
    }

    if (mpi_size > 1) {
        MPI_Bcast(&rc, 1, MPI_INT, 0, comm);

        if (rank != 0) {
            if (rc)
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                                   MPIR_ERR_RECOVERABLE, myname,
                                                   __LINE__, rc, "File Open error", 0);
            else
                *error_code = MPI_SUCCESS;
        }
    }
    if (*error_code != MPI_SUCCESS)
        goto err_free;

    if (mpi_size > 1) {
        rc = share_cont_info(cont, rank, comm);
        if (rc) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, myname,
                                               __LINE__, rc, "File Open error", 0);
            goto err_free;
        }
    }

    fd->is_open = 1;
    fd->access_mode = orig_amode_wronly;

#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event(ADIOI_MPE_open_b, 0, NULL);
#endif

    return;

  err_free:
    if (cont->obj_name)
        ADIOI_Free(cont->obj_name);
    if (cont->cont_name)
        ADIOI_Free(cont->cont_name);
    ADIOI_Free(cont);
    return;
}

void ADIOI_DAOS_Flush(ADIO_File fd, int *error_code)
{
    MPI_Barrier(fd->comm);
    *error_code = MPI_SUCCESS;
}

void ADIOI_DAOS_Delete(const char *filename, int *error_code)
{
    struct adio_daos_hdl *p, *c;
    uuid_t puuid, cuuid;
    dfs_t *dfs;
    char *obj_name, *cont_name;
    daos_oclass_id_t oclass;
    daos_size_t chunk_size;
    static char myname[] = "ADIOI_DAOS_DELETE";
    int rc;

    ADIOI_DAOS_Init(error_code);
    if (*error_code != MPI_SUCCESS)
        return;

    parse_filename(filename, &obj_name, &cont_name);
    if (rc) {
        *error_code = MPI_ERR_NO_MEM;
        return;
    }

    rc = get_pool_cont_uuids(cont_name, &puuid, &cuuid, &oclass, &chunk_size);
    if (rc) {
        *error_code = ADIOI_DAOS_err(myname, cont_name, __LINE__, rc);
        return;
    }

    rc = adio_daos_poh_lookup_connect(puuid, &p);
    if (rc || p == NULL) {
        PRINT_MSG(stderr, "Failed to connect to pool\n");
        *error_code = ADIOI_DAOS_err(myname, cont_name, __LINE__, rc);
        goto out_free;
    }

    rc = adio_daos_coh_lookup_create(p->open_hdl, cuuid, O_RDWR, false, &c);
    if (rc || c == NULL) {
        *error_code = ADIOI_DAOS_err(myname, cont_name, __LINE__, rc);
        goto out_pool;
    }

    if (c->dfs == NULL) {
        /* Mount a flat namespace on the container */
        rc = dfs_mount(p->open_hdl, c->open_hdl, O_RDWR, &dfs);
        if (rc) {
            PRINT_MSG(stderr, "Failed to mount flat namespace (%d)\n", rc);
            *error_code = ADIOI_DAOS_err(myname, obj_name, __LINE__, rc);
            goto out_cont;
        }
        c->dfs = dfs;
    }

    /* Remove the file from the flat namespace */
    rc = dfs_remove(c->dfs, NULL, obj_name, true, NULL);
    if (rc) {
        *error_code = ADIOI_DAOS_err(myname, obj_name, __LINE__, rc);
        goto out_cont;
    }

    *error_code = MPI_SUCCESS;

  out_cont:
    adio_daos_coh_release(c);
  out_pool:
    adio_daos_poh_release(p);
  out_free:
    ADIOI_Free(obj_name);
    ADIOI_Free(cont_name);
    return;
}

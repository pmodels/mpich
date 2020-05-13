/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "ad_nfs.h"
#include "adio_extern.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void ADIOI_NFS_WriteContig(ADIO_File fd, const void *buf, int count,
                           MPI_Datatype datatype, int file_ptr_type,
                           ADIO_Offset offset, ADIO_Status * status, int *error_code)
{
    ssize_t err = -1;
    MPI_Count datatype_size, len;
    ADIO_Offset bytes_xfered = 0;
    size_t wr_count;
    static char myname[] = "ADIOI_NFS_WRITECONTIG";
    char *p;

    if (count == 0) {
        err = 0;
        goto fn_exit;
    }

    MPI_Type_size_x(datatype, &datatype_size);
    len = datatype_size * (ADIO_Offset) count;

    if (file_ptr_type == ADIO_INDIVIDUAL) {
        offset = fd->fp_ind;
    }

    p = (char *) buf;
    while (bytes_xfered < len) {
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);
#endif
        wr_count = len - bytes_xfered;
        /* work around FreeBSD and OS X defects */
        if (wr_count > INT_MAX)
            wr_count = INT_MAX;

        ADIOI_WRITE_LOCK(fd, offset + bytes_xfered, SEEK_SET, wr_count);
        err = pwrite(fd->fd_sys, p, wr_count, offset + bytes_xfered);
        /* --BEGIN ERROR HANDLING-- */
        if (err == -1) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__, MPI_ERR_IO, "**io",
                                               "**io %s", strerror(errno));
            fd->fp_sys_posn = -1;
            return;
        }
        /* --END ERROR HANDLING-- */
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);
#endif
        ADIOI_UNLOCK(fd, offset + bytes_xfered, SEEK_SET, wr_count);
        bytes_xfered += err;
        p += err;
    }
    fd->fp_sys_posn = offset + bytes_xfered;

    if (file_ptr_type == ADIO_INDIVIDUAL) {
        fd->fp_ind += bytes_xfered;
    }

  fn_exit:
#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, bytes_xfered);
#endif

    *error_code = MPI_SUCCESS;
}




#ifdef ADIOI_MPE_LOGGING
#define ADIOI_BUFFERED_WRITE                                            \
    {                                                                   \
        if (req_off >= writebuf_off + writebuf_len) {                   \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            writebuf_off = req_off;                                     \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            MPE_Log_event(ADIOI_MPE_read_a, 0, NULL);                   \
            err = read(fd->fd_sys, writebuf, writebuf_len);             \
            MPE_Log_event(ADIOI_MPE_read_b, 0, NULL);                   \
            if (err == -1) {                                            \
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,         \
                                                   MPIR_ERR_RECOVERABLE, myname, \
                                                   __LINE__, MPI_ERR_IO, \
                                                   "**ioRMWrdwr", 0);   \
                goto fn_exit;                                           \
            }                                                           \
        }                                                               \
        write_sz = (int) (MPL_MIN(req_len, writebuf_off + writebuf_len - req_off)); \
        memcpy(writebuf+req_off-writebuf_off, (char *)buf +userbuf_off, write_sz); \
        while (write_sz != req_len) {                                   \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            req_len -= write_sz;                                        \
            userbuf_off += write_sz;                                    \
            writebuf_off += writebuf_len;                               \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            MPE_Log_event(ADIOI_MPE_read_a, 0, NULL);                   \
            err = read(fd->fd_sys, writebuf, writebuf_len);             \
            MPE_Log_event(ADIOI_MPE_read_b, 0, NULL);                   \
            if (err == -1) {                                            \
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,         \
                                                   MPIR_ERR_RECOVERABLE, myname, \
                                                   __LINE__, MPI_ERR_IO, \
                                                   "**ioRMWrdwr", 0);   \
                goto fn_exit;                                           \
            }                                                           \
            write_sz = MPL_MIN(req_len, writebuf_len);                  \
            memcpy(writebuf, (char *)buf + userbuf_off, write_sz);      \
        }                                                               \
    }
#else
#define ADIOI_BUFFERED_WRITE                                            \
    {                                                                   \
        if (req_off >= writebuf_off + writebuf_len) {                   \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            writebuf_off = req_off;                                     \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            err = read(fd->fd_sys, writebuf, writebuf_len);             \
            if (err == -1) {                                            \
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,         \
                                                   MPIR_ERR_RECOVERABLE, myname, \
                                                   __LINE__, MPI_ERR_IO, \
                                                   "**ioRMWrdwr", 0);   \
                goto fn_exit;                                           \
            }                                                           \
        }                                                               \
        write_sz = (int) (MPL_MIN(req_len, writebuf_off + writebuf_len - req_off)); \
        memcpy(writebuf+req_off-writebuf_off, (char *)buf +userbuf_off, write_sz); \
        while (write_sz != req_len) {                                   \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            req_len -= write_sz;                                        \
            userbuf_off += write_sz;                                    \
            writebuf_off += writebuf_len;                               \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            err = read(fd->fd_sys, writebuf, writebuf_len);             \
            if (err == -1) {                                            \
                *error_code = MPIO_Err_create_code(MPI_SUCCESS,         \
                                                   MPIR_ERR_RECOVERABLE, myname, \
                                                   __LINE__, MPI_ERR_IO, \
                                                   "**ioRMWrdwr", 0);   \
                goto fn_exit;                                           \
            }                                                           \
            write_sz = MPL_MIN(req_len, writebuf_len);                  \
            memcpy(writebuf, (char *)buf + userbuf_off, write_sz);      \
        }                                                               \
    }
#endif

/* this macro is used when filetype is contig and buftype is not contig.
   it does not do a read-modify-write and does not lock*/
#ifdef ADIOI_MPE_LOGGING
#define ADIOI_BUFFERED_WRITE_WITHOUT_READ                               \
    {                                                                   \
        if (req_off >= writebuf_off + writebuf_len) {                   \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            writebuf_off = req_off;                                     \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
        }                                                               \
        write_sz = (int) (MPL_MIN(req_len, writebuf_off + writebuf_len - req_off)); \
        memcpy(writebuf+req_off-writebuf_off, (char *)buf +userbuf_off, write_sz); \
        while (write_sz != req_len) {                                   \
            MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);                  \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);                  \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);                  \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            req_len -= write_sz;                                        \
            userbuf_off += write_sz;                                    \
            writebuf_off += writebuf_len;                               \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            write_sz = MPL_MIN(req_len, writebuf_len);                  \
            memcpy(writebuf, (char *)buf + userbuf_off, write_sz);      \
        }                                                               \
    }
#else
#define ADIOI_BUFFERED_WRITE_WITHOUT_READ                               \
    {                                                                   \
        if (req_off >= writebuf_off + writebuf_len) {                   \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            writebuf_off = req_off;                                     \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
        }                                                               \
        write_sz = (int) (MPL_MIN(req_len, writebuf_off + writebuf_len - req_off)); \
        memcpy(writebuf+req_off-writebuf_off, (char *)buf +userbuf_off, write_sz); \
        while (write_sz != req_len) {                                   \
            lseek(fd->fd_sys, writebuf_off, SEEK_SET);                  \
            if (!(fd->atomicity)) ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            err = write(fd->fd_sys, writebuf, writebuf_len);            \
            if (!(fd->atomicity)) ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len); \
            if (err == -1) err_flag = 1;                                \
            req_len -= write_sz;                                        \
            userbuf_off += write_sz;                                    \
            writebuf_off += writebuf_len;                               \
            writebuf_len = (int) (MPL_MIN(max_bufsize,end_offset-writebuf_off+1)); \
            write_sz = MPL_MIN(req_len, writebuf_len);                  \
            memcpy(writebuf, (char *)buf + userbuf_off, write_sz);      \
        }                                                               \
    }
#endif


void ADIOI_NFS_WriteStrided(ADIO_File fd, const void *buf, int count,
                            MPI_Datatype datatype, int file_ptr_type,
                            ADIO_Offset offset, ADIO_Status * status, int
                            *error_code)
{
/* offset is in units of etype relative to the filetype. */

    ADIOI_Flatlist_node *flat_buf, *flat_file;
    int i, j, k, err = -1, bwr_size, st_index = 0;
    ADIO_Offset i_offset, sum, size_in_filetype;
    ADIO_Offset num, size, n_etypes_in_filetype;
    MPI_Count bufsize;
    ADIO_Offset n_filetypes, etype_in_filetype;
    ADIO_Offset abs_off_in_filetype = 0;
    int req_len;
    MPI_Count filetype_size, etype_size, buftype_size;
    MPI_Aint filetype_extent, buftype_extent;
    int buf_count, buftype_is_contig, filetype_is_contig;
    ADIO_Offset userbuf_off;
    ADIO_Offset off, req_off, disp, end_offset = 0, writebuf_off, start_off;
    char *writebuf = NULL, *value;
    int st_n_filetypes, writebuf_len, write_sz;
    ADIO_Offset fwr_size = 0, new_fwr_size, st_fwr_size;
    int new_bwr_size, err_flag = 0, info_flag, max_bufsize;
    static char myname[] = "ADIOI_NFS_WRITESTRIDED";

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fd->filetype, &filetype_is_contig);

    MPI_Type_size_x(fd->filetype, &filetype_size);
    if (!filetype_size) {
#ifdef HAVE_STATUS_SET_BYTES
        MPIR_Status_set_bytes(status, datatype, 0);
#endif
        *error_code = MPI_SUCCESS;
        return;
    }

    MPI_Type_extent(fd->filetype, &filetype_extent);
    MPI_Type_size_x(datatype, &buftype_size);
    MPI_Type_extent(datatype, &buftype_extent);
    etype_size = fd->etype_size;

    bufsize = buftype_size * count;

/* get max_bufsize from the info object. */

    value = (char *) ADIOI_Malloc((MPI_MAX_INFO_VAL + 1) * sizeof(char));
    ADIOI_Info_get(fd->info, "ind_wr_buffer_size", MPI_MAX_INFO_VAL, value, &info_flag);
    max_bufsize = atoi(value);
    ADIOI_Free(value);

    if (!buftype_is_contig && filetype_is_contig) {

/* noncontiguous in memory, contiguous in file. */

        flat_buf = ADIOI_Flatten_and_find(datatype);

        off = (file_ptr_type == ADIO_INDIVIDUAL) ? fd->fp_ind : fd->disp + etype_size * offset;

        start_off = off;
        end_offset = off + bufsize - 1;
        writebuf_off = off;
        writebuf = (char *) ADIOI_Malloc(max_bufsize);
        writebuf_len = (int) (MPL_MIN(max_bufsize, end_offset - writebuf_off + 1));

/* if atomicity is true, lock the region to be accessed */
        if (fd->atomicity)
            ADIOI_WRITE_LOCK(fd, start_off, SEEK_SET, end_offset - start_off + 1);

        for (j = 0; j < count; j++)
            for (i = 0; i < flat_buf->count; i++) {
                userbuf_off = j * buftype_extent + flat_buf->indices[i];
                req_off = off;
                req_len = flat_buf->blocklens[i];
                ADIOI_BUFFERED_WRITE_WITHOUT_READ;
                off += flat_buf->blocklens[i];
            }

        /* write the buffer out finally */
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);
#endif
        lseek(fd->fd_sys, writebuf_off, SEEK_SET);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);
#endif
        if (!(fd->atomicity))
            ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);
#endif
        err = write(fd->fd_sys, writebuf, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);
#endif
        if (!(fd->atomicity))
            ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len);
        if (err == -1)
            err_flag = 1;

        if (fd->atomicity)
            ADIOI_UNLOCK(fd, start_off, SEEK_SET, end_offset - start_off + 1);

        if (file_ptr_type == ADIO_INDIVIDUAL)
            fd->fp_ind = off;
        if (err_flag) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE, myname,
                                               __LINE__, MPI_ERR_IO, "**io",
                                               "**io %s", strerror(errno));
        } else
            *error_code = MPI_SUCCESS;
    }

    else {      /* noncontiguous in file */

        flat_file = ADIOI_Flatten_and_find(fd->filetype);
        disp = fd->disp;

        if (file_ptr_type == ADIO_INDIVIDUAL) {
            /* Wei-keng reworked type processing to be a bit more efficient */
            offset = fd->fp_ind - disp;
            n_filetypes = (offset - flat_file->indices[0]) / filetype_extent;
            offset -= (ADIO_Offset) n_filetypes *filetype_extent;
            /* now offset is local to this extent */

            /* find the block where offset is located, skip blocklens[i]==0 */
            for (i = 0; i < flat_file->count; i++) {
                ADIO_Offset dist;
                if (flat_file->blocklens[i] == 0)
                    continue;
                dist = flat_file->indices[i] + flat_file->blocklens[i] - offset;
                /* fwr_size is from offset to the end of block i */
                if (dist == 0) {
                    i++;
                    offset = flat_file->indices[i];
                    fwr_size = flat_file->blocklens[i];
                    break;
                }
                if (dist > 0) {
                    fwr_size = dist;
                    break;
                }
            }
            st_index = i;       /* starting index in flat_file->indices[] */
            offset += disp + (ADIO_Offset) n_filetypes *filetype_extent;
        } else {
            n_etypes_in_filetype = filetype_size / etype_size;
            n_filetypes = offset / n_etypes_in_filetype;
            etype_in_filetype = offset % n_etypes_in_filetype;
            size_in_filetype = etype_in_filetype * etype_size;

            sum = 0;
            for (i = 0; i < flat_file->count; i++) {
                sum += flat_file->blocklens[i];
                if (sum > size_in_filetype) {
                    st_index = i;
                    fwr_size = sum - size_in_filetype;
                    abs_off_in_filetype = flat_file->indices[i] +
                        size_in_filetype - (sum - flat_file->blocklens[i]);
                    break;
                }
            }

            /* abs. offset in bytes in the file */
            offset = disp + (ADIO_Offset) n_filetypes *filetype_extent + abs_off_in_filetype;
        }

        start_off = offset;
        /* Wei-keng Liao:write request is within single flat_file contig block */
        /* this could happen, for example, with subarray types that are
         * actually fairly contiguous */
        if (buftype_is_contig && bufsize <= fwr_size) {
            /* though MPI api has an integer 'count' parameter, derived
             * datatypes might describe more bytes than can fit into an integer.
             * if we've made it this far, we can pass a count of original
             * datatypes, instead of a count of bytes (which might overflow)
             * Other WriteContig calls in this path are operating on data
             * sieving buffer */
            ADIO_WriteContig(fd, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
                             offset, status, error_code);

            if (file_ptr_type == ADIO_INDIVIDUAL) {
                /* update MPI-IO file pointer to point to the first byte
                 * that can be accessed in the fileview. */
                fd->fp_ind = offset + bufsize;
                if (bufsize == fwr_size) {
                    do {
                        st_index++;
                        if (st_index == flat_file->count) {
                            st_index = 0;
                            n_filetypes++;
                        }
                    } while (flat_file->blocklens[st_index] == 0);
                    fd->fp_ind = disp + flat_file->indices[st_index]
                    + (ADIO_Offset) n_filetypes *filetype_extent;
                }
            }
            fd->fp_sys_posn = -1;       /* set it to null. */
#ifdef HAVE_STATUS_SET_BYTES
            MPIR_Status_set_bytes(status, datatype, bufsize);
#endif
            goto fn_exit;
        }

        /* Calculate end_offset, the last byte-offset that will be accessed.
         * e.g., if start_offset=0 and 100 bytes to be write, end_offset=99 */

        st_fwr_size = fwr_size;
        st_n_filetypes = n_filetypes;
        i_offset = 0;
        j = st_index;
        off = offset;
        fwr_size = MPL_MIN(st_fwr_size, bufsize);
        while (i_offset < bufsize) {
            i_offset += fwr_size;
            end_offset = off + fwr_size - 1;

            j = (j + 1) % flat_file->count;
            n_filetypes += (j == 0) ? 1 : 0;
            while (flat_file->blocklens[j] == 0) {
                j = (j + 1) % flat_file->count;
                n_filetypes += (j == 0) ? 1 : 0;
            }

            off = disp + flat_file->indices[j] + n_filetypes * (ADIO_Offset) filetype_extent;
            fwr_size = MPL_MIN(flat_file->blocklens[j], bufsize - i_offset);
        }

/* if atomicity is true, lock the region to be accessed */
        if (fd->atomicity)
            ADIOI_WRITE_LOCK(fd, start_off, SEEK_SET, end_offset - start_off + 1);

        /* initial read for the read-modify-write */
        writebuf_off = offset;
        writebuf = (char *) ADIOI_Malloc(max_bufsize);
        memset(writebuf, -1, max_bufsize);
        writebuf_len = (int) (MPL_MIN(max_bufsize, end_offset - writebuf_off + 1));
        if (!(fd->atomicity))
            ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);
#endif
        lseek(fd->fd_sys, writebuf_off, SEEK_SET);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);
#endif
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_read_a, 0, NULL);
#endif
        err = read(fd->fd_sys, writebuf, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_read_b, 0, NULL);
#endif
        if (err == -1) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE,
                                               myname, __LINE__,
                                               MPI_ERR_IO,
                                               "ADIOI_NFS_WriteStrided: ROMIO tries to optimize this access by doing a read-modify-write, but is unable to read the file. Please give the file read permission and open it with MPI_MODE_RDWR.",
                                               0);
            goto fn_exit;
        }

        if (buftype_is_contig && !filetype_is_contig) {

/* contiguous in memory, noncontiguous in file. should be the most
   common case. */

            i_offset = 0;
            j = st_index;
            off = offset;
            n_filetypes = st_n_filetypes;
            fwr_size = MPL_MIN(st_fwr_size, bufsize);
            while (i_offset < bufsize) {
                if (fwr_size) {
                    /* TYPE_UB and TYPE_LB can result in
                     * fwr_size = 0. save system call in such cases */
                    /* lseek(fd->fd_sys, off, SEEK_SET);
                     * err = write(fd->fd_sys, ((char *) buf) + i, fwr_size); */

                    req_off = off;
                    req_len = fwr_size;
                    userbuf_off = i_offset;
                    ADIOI_BUFFERED_WRITE;
                }
                i_offset += fwr_size;

                if (off + fwr_size < disp + flat_file->indices[j] +
                    flat_file->blocklens[j] + n_filetypes * (ADIO_Offset) filetype_extent)
                    off += fwr_size;
                /* did not reach end of contiguous block in filetype.
                 * no more I/O needed. off is incremented by fwr_size. */
                else {
                    j = (j + 1) % flat_file->count;
                    n_filetypes += (j == 0) ? 1 : 0;
                    while (flat_file->blocklens[j] == 0) {
                        j = (j + 1) % flat_file->count;
                        n_filetypes += (j == 0) ? 1 : 0;
                    }
                    off = disp + flat_file->indices[j] +
                        n_filetypes * (ADIO_Offset) filetype_extent;
                    fwr_size = MPL_MIN(flat_file->blocklens[j], bufsize - i_offset);
                }
            }
        } else {
/* noncontiguous in memory as well as in file */

            flat_buf = ADIOI_Flatten_and_find(datatype);

            k = num = buf_count = 0;
            i_offset = flat_buf->indices[0];
            j = st_index;
            off = offset;
            n_filetypes = st_n_filetypes;
            fwr_size = st_fwr_size;
            bwr_size = flat_buf->blocklens[0];

            while (num < bufsize) {
                size = MPL_MIN(fwr_size, bwr_size);
                if (size) {
                    /* lseek(fd->fd_sys, off, SEEK_SET);
                     * err = write(fd->fd_sys, ((char *) buf) + i, size); */

                    req_off = off;
                    req_len = size;
                    userbuf_off = i_offset;
                    ADIOI_BUFFERED_WRITE;
                }

                new_fwr_size = fwr_size;
                new_bwr_size = bwr_size;

                if (size == fwr_size) {
/* reached end of contiguous block in file */
                    j = (j + 1) % flat_file->count;
                    n_filetypes += (j == 0) ? 1 : 0;
                    while (flat_file->blocklens[j] == 0) {
                        j = (j + 1) % flat_file->count;
                        n_filetypes += (j == 0) ? 1 : 0;
                    }

                    off = disp + flat_file->indices[j] +
                        n_filetypes * (ADIO_Offset) filetype_extent;

                    new_fwr_size = flat_file->blocklens[j];
                    if (size != bwr_size) {
                        i_offset += size;
                        new_bwr_size -= size;
                    }
                }

                if (size == bwr_size) {
/* reached end of contiguous block in memory */

                    k = (k + 1) % flat_buf->count;
                    buf_count++;
                    i_offset =
                        (ADIO_Offset) buftype_extent *(ADIO_Offset) (buf_count / flat_buf->count) +
                        flat_buf->indices[k];
                    new_bwr_size = flat_buf->blocklens[k];
                    if (size != fwr_size) {
                        off += size;
                        new_fwr_size -= size;
                    }
                }
                num += size;
                fwr_size = new_fwr_size;
                bwr_size = new_bwr_size;
            }
        }

        /* write the buffer out finally */
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_a, 0, NULL);
#endif
        lseek(fd->fd_sys, writebuf_off, SEEK_SET);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_lseek_b, 0, NULL);
#endif
        if (!(fd->atomicity))
            ADIOI_WRITE_LOCK(fd, writebuf_off, SEEK_SET, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_a, 0, NULL);
#endif
        err = write(fd->fd_sys, writebuf, writebuf_len);
#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_write_b, 0, NULL);
#endif

        if (!(fd->atomicity))
            ADIOI_UNLOCK(fd, writebuf_off, SEEK_SET, writebuf_len);
        else
            ADIOI_UNLOCK(fd, start_off, SEEK_SET, end_offset - start_off + 1);

        if (err == -1)
            err_flag = 1;

        if (file_ptr_type == ADIO_INDIVIDUAL)
            fd->fp_ind = off;
        if (err_flag) {
            *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                               MPIR_ERR_RECOVERABLE, myname,
                                               __LINE__, MPI_ERR_IO, "**io",
                                               "**io %s", strerror(errno));
        } else
            *error_code = MPI_SUCCESS;
    }

    fd->fp_sys_posn = -1;       /* set it to null. */

#ifdef HAVE_STATUS_SET_BYTES
    MPIR_Status_set_bytes(status, datatype, bufsize);
/* This is a temporary way of filling in status. The right way is to
   keep track of how much data was actually written by ADIOI_BUFFERED_WRITE. */
#endif

  fn_exit:
    if (writebuf != NULL)
        ADIOI_Free(writebuf);

    return;
}

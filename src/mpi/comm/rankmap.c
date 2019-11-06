/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

enum src_mapper_models {
    SRC_MAPPER_IRREGULAR = 0,
    SRC_MAPPER_DIRECT = 1,
    SRC_MAPPER_OFFSET = 2,
    SRC_MAPPER_STRIDE = 3
};

static int map_size(MPIR_Comm_map_t map);
static int detect_regular_model(int *lpid, int size, int *offset, int *blocksize, int *stride);
static int src_comm_to_mlut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest, int size,
                            int total_mapper_size, int mapper_offset);
static int src_mlut_to_mlut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                            MPIR_Comm_map_t * mapper, int total_mapper_size, int mapper_offset);
static int src_map_to_lut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest, MPIR_Comm_map_t * mapper,
                          int total_mapper_size, int mapper_offset);
static void direct_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper);
static void offset_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int offset);
static void stride_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int stride, int blocksize, int offset);
static int check_convert_mlut_to_lut(MPIR_rank_map_t * src);
static int check_convert_lut_to_regular(MPIR_rank_map_t * src);
static int set_map(MPIR_rank_map_t * src_rmap, MPIR_rank_map_t * dest_rmap,
                   MPIR_Comm_map_t * mapper, int src_comm_size, int total_mapper_size,
                   int mapper_offset);

static int map_size(MPIR_Comm_map_t map)
{
    int ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_MAP_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_MAP_SIZE);

    if (map.type == MPIR_COMM_MAP_TYPE__IRREGULAR)
        ret = map.src_mapping_size;
    else if (map.dir == MPIR_COMM_MAP_DIR__L2L || map.dir == MPIR_COMM_MAP_DIR__L2R)
        ret = map.src_comm->local_size;
    else
        ret = map.src_comm->remote_size;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_MAP_SIZE);
    return ret;
}

static int detect_regular_model(int *lpid, int size, int *offset, int *blocksize, int *stride)
{
    int off = 0, bs = 0, st = 0;
    int i;
    int ret = SRC_MAPPER_IRREGULAR;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_DETECT_REGULAR_MODEL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_DETECT_REGULAR_MODEL);

    off = lpid[0];
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: offset %d", off));

    for (i = 0; i < size; i++) {
        if (lpid[i] != i + off) {
            break;
        }
        bs++;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: blocksize %d", bs));
    if (bs == size) {
        if (off == 0) {
            ret = SRC_MAPPER_DIRECT;
            goto fn_exit;
        } else {
            *offset = off;
            ret = SRC_MAPPER_OFFSET;
            goto fn_exit;
        }
    }

    /* blocksize less than total size, try if this is stride */
    st = lpid[bs] - lpid[0];
    if (st < 0 || st <= bs) {
        ret = SRC_MAPPER_IRREGULAR;
        goto fn_exit;
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: stride %d", st));
    for (i = bs; i < size; i++) {
        if (lpid[i] != MPIR_CALC_STRIDE(i, st, bs, off)) {
            ret = SRC_MAPPER_IRREGULAR;
            goto fn_exit;
        }
    }
    *offset = off;
    *blocksize = bs;
    *stride = st;
    ret = SRC_MAPPER_STRIDE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_DETECT_REGULAR_MODEL);
    return ret;
}

static int src_comm_to_mlut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest, int size,
                            int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SRC_COMM_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SRC_COMM_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIR_alloc_mlut(&mlut, total_mapper_size);
        MPIR_ERR_CHECK(mpi_errno);
        dest->size = total_mapper_size;
        dest->mode = MPIR_RANK_MAP_MLUT;
        dest->avtid = -1;
        dest->irreg.mlut.t = mlut;
        dest->irreg.mlut.gpid = mlut->gpid;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " size %d", size));
    switch (src->mode) {
        case MPIR_RANK_MAP_DIRECT:
        case MPIR_RANK_MAP_DIRECT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = i;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            break;
        case MPIR_RANK_MAP_OFFSET:
        case MPIR_RANK_MAP_OFFSET_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = i + src->reg.offset;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
            break;
        case MPIR_RANK_MAP_STRIDE:
        case MPIR_RANK_MAP_STRIDE_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                    MPIR_CALC_STRIDE_SIMPLE(i, src->reg.stride.stride, src->reg.stride.offset);
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIR_RANK_MAP_STRIDE_BLOCK:
        case MPIR_RANK_MAP_STRIDE_BLOCK_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                    MPIR_CALC_STRIDE(i, src->reg.stride.stride, src->reg.stride.blocksize,
                                     src->reg.stride.offset);
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIR_RANK_MAP_LUT:
        case MPIR_RANK_MAP_LUT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[i];
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            break;
        case MPIR_RANK_MAP_MLUT:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[i].lpid;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[i].avtid;
            }
            break;
        case MPIR_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SRC_COMM_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int src_mlut_to_mlut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                            MPIR_Comm_map_t * mapper, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = map_size(*mapper);
    MPIR_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SRC_MLUT_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SRC_MLUT_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIR_alloc_mlut(&mlut, total_mapper_size);
        MPIR_ERR_CHECK(mpi_errno);
        dest->size = total_mapper_size;
    }

    dest->mode = src->mode;
    dest->irreg.mlut.t = mlut;
    dest->irreg.mlut.gpid = mlut->gpid;
    for (i = 0; i < size; i++) {
        dest->irreg.mlut.gpid[i + mapper_offset].avtid =
            src->irreg.mlut.gpid[mapper->src_mapping[i]].avtid;
        dest->irreg.mlut.gpid[i + mapper_offset].lpid =
            src->irreg.mlut.gpid[mapper->src_mapping[i]].lpid;
    }
  fn_exit:
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " src mode %d, dest mode %d",
                                             (int) src->mode, (int) dest->mode));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SRC_MLUT_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int src_map_to_lut(MPIR_rank_map_t * src, MPIR_rank_map_t * dest, MPIR_Comm_map_t * mapper,
                          int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = map_size(*mapper);
    MPIR_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SRC_MAP_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SRC_MAP_TO_LUT);

    if (!mapper_offset) {
        mpi_errno = MPIR_alloc_lut(&lut, total_mapper_size);
        MPIR_ERR_CHECK(mpi_errno);
        dest->size = total_mapper_size;
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " size %d, mapper->src_mapping_size %d",
                                             size, mapper->src_mapping_size));
    dest->mode = MPIR_RANK_MAP_LUT;
    dest->avtid = src->avtid;
    dest->irreg.lut.t = lut;
    dest->irreg.lut.lpid = lut->lpid;
    switch (src->mode) {
        case MPIR_RANK_MAP_DIRECT:
        case MPIR_RANK_MAP_DIRECT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i];
            }
            break;
        case MPIR_RANK_MAP_OFFSET:
        case MPIR_RANK_MAP_OFFSET_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i] + src->reg.offset;
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source offset %d",
                                                     src->reg.offset));
            break;
        case MPIR_RANK_MAP_STRIDE:
        case MPIR_RANK_MAP_STRIDE_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] =
                    MPIR_CALC_STRIDE_SIMPLE(mapper->src_mapping[i], src->reg.stride.stride,
                                            src->reg.stride.offset);
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIR_RANK_MAP_STRIDE_BLOCK:
        case MPIR_RANK_MAP_STRIDE_BLOCK_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = MPIR_CALC_STRIDE(mapper->src_mapping[i],
                                                                           src->reg.stride.stride,
                                                                           src->reg.
                                                                           stride.blocksize,
                                                                           src->reg.stride.offset);
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIR_RANK_MAP_LUT:
        case MPIR_RANK_MAP_LUT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] =
                    src->irreg.lut.lpid[mapper->src_mapping[i]];
            }
            break;
        default:
            mpi_errno = 1;
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, " cannot convert mode %d to lut", (int) src->mode));
            goto fn_fail;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SRC_MAP_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void direct_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_DIRECT_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_DIRECT_OF_SRC_RMAP);
    dest->mode = src->mode;
    if (mapper) {
        dest->size = map_size(*mapper);
    } else {
        dest->size = src->size;
    }
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIR_RANK_MAP_DIRECT:
        case MPIR_RANK_MAP_DIRECT_INTRA:
            break;
        case MPIR_RANK_MAP_OFFSET:
        case MPIR_RANK_MAP_OFFSET_INTRA:
            dest->reg.offset = src->reg.offset;
            break;
        case MPIR_RANK_MAP_STRIDE:
        case MPIR_RANK_MAP_STRIDE_INTRA:
        case MPIR_RANK_MAP_STRIDE_BLOCK:
        case MPIR_RANK_MAP_STRIDE_BLOCK_INTRA:
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset;
            break;
        case MPIR_RANK_MAP_LUT:
        case MPIR_RANK_MAP_LUT_INTRA:
            dest->irreg.lut.t = src->irreg.lut.t;
            dest->irreg.lut.lpid = src->irreg.lut.lpid;
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tref count %d",
                                                     MPIR_Object_get_ref(src->irreg.lut.t)));
            MPIR_Object_add_ref(src->irreg.lut.t);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
            break;
        case MPIR_RANK_MAP_MLUT:
            dest->irreg.mlut.t = src->irreg.mlut.t;
            dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
            MPIR_Object_add_ref(src->irreg.mlut.t);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
            break;
        case MPIR_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_DIRECT_OF_SRC_RMAP);
}

static void offset_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_OFFSET_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_OFFSET_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    dest->size = map_size(*mapper);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIR_RANK_MAP_DIRECT_INTRA:
            dest->mode = MPIR_RANK_MAP_OFFSET_INTRA;
            dest->reg.offset = offset;
            break;
        case MPIR_RANK_MAP_DIRECT:
            dest->mode = MPIR_RANK_MAP_OFFSET;
            dest->reg.offset = offset;
            break;
        case MPIR_RANK_MAP_OFFSET:
            dest->mode = MPIR_RANK_MAP_OFFSET;
            dest->reg.offset = src->reg.offset + offset;
            break;
        case MPIR_RANK_MAP_OFFSET_INTRA:
            dest->mode = MPIR_RANK_MAP_OFFSET_INTRA;
            dest->reg.offset = src->reg.offset + offset;
            break;
        case MPIR_RANK_MAP_STRIDE:
            dest->mode = MPIR_RANK_MAP_STRIDE;
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
            break;
        case MPIR_RANK_MAP_STRIDE_INTRA:
            dest->mode = MPIR_RANK_MAP_STRIDE_INTRA;
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
            break;
        case MPIR_RANK_MAP_STRIDE_BLOCK:
        case MPIR_RANK_MAP_STRIDE_BLOCK_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIR_RANK_MAP_LUT:
        case MPIR_RANK_MAP_LUT_INTRA:
            dest->mode = src->mode;
            dest->irreg.lut.t = src->irreg.lut.t;
            dest->irreg.lut.lpid = &src->irreg.lut.lpid[offset];
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tref count %d",
                                                     MPIR_Object_get_ref(src->irreg.lut.t)));
            MPIR_Object_add_ref(src->irreg.lut.t);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
            break;
        case MPIR_RANK_MAP_MLUT:
            dest->mode = src->mode;
            dest->irreg.mlut.t = src->irreg.mlut.t;
            dest->irreg.mlut.gpid = &src->irreg.mlut.gpid[offset];
            MPIR_Object_add_ref(src->irreg.mlut.t);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
            break;
        case MPIR_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_OFFSET_OF_SRC_RMAP);
}

static void stride_of_src_rmap(MPIR_rank_map_t * src, MPIR_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int stride, int blocksize, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_STRIDE_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_STRIDE_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIR_RANK_MAP_DIRECT_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE_INTRA;
            } else {
                dest->mode = MPIR_RANK_MAP_STRIDE_BLOCK_INTRA;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset;
            MPIR_Assert(stride > 0);
            MPIR_Assert(blocksize > 0);
            break;
        case MPIR_RANK_MAP_DIRECT:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE;
            } else {
                dest->mode = MPIR_RANK_MAP_STRIDE_BLOCK;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset;
            MPIR_Assert(stride > 0);
            MPIR_Assert(blocksize > 0);
            break;
        case MPIR_RANK_MAP_OFFSET:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE;
            } else {
                dest->mode = MPIR_RANK_MAP_STRIDE_BLOCK;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset + src->reg.offset;
            break;
        case MPIR_RANK_MAP_OFFSET_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE_INTRA;
            } else {
                dest->mode = MPIR_RANK_MAP_STRIDE_BLOCK_INTRA;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset + src->reg.offset;
            break;
        case MPIR_RANK_MAP_STRIDE:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE;
                dest->reg.stride.stride = src->reg.stride.stride * stride;
                dest->reg.stride.blocksize = blocksize;
                dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
            } else {
                src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            }
            break;
        case MPIR_RANK_MAP_STRIDE_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIR_RANK_MAP_STRIDE_INTRA;
                dest->reg.stride.stride = src->reg.stride.stride * stride;
                dest->reg.stride.blocksize = blocksize;
                dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
            } else {
                src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            }
            break;
        case MPIR_RANK_MAP_STRIDE_BLOCK:
        case MPIR_RANK_MAP_STRIDE_BLOCK_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIR_RANK_MAP_LUT:
        case MPIR_RANK_MAP_LUT_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIR_RANK_MAP_MLUT:
            src_mlut_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIR_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_STRIDE_OF_SRC_RMAP);
}

static int check_convert_mlut_to_lut(MPIR_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS, i;
    int flag = 1;
    int avtid;
    MPIR_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_CHECK_CONVERT_MLUT_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_CHECK_CONVERT_MLUT_TO_LUT);

    if (src->mode != MPIR_RANK_MAP_MLUT) {
        goto fn_exit;
    }

    /* check if all mlut item has the same avtid */
    avtid = src->irreg.mlut.gpid[0].avtid;
    for (i = 1; i < src->size; i++) {
        if (src->irreg.mlut.gpid[i].avtid != avtid) {
            flag = 0;
            break;
        }
    }
    if (!flag) {        /* multiple avtid */
        goto fn_exit;
    }

    src->avtid = avtid;
    if (avtid == 0) {
        src->mode = MPIR_RANK_MAP_LUT_INTRA;
    } else {
        src->mode = MPIR_RANK_MAP_LUT;
    }
    mpi_errno = MPIR_alloc_lut(&lut, src->size);
    MPIR_ERR_CHECK(mpi_errno);
    for (i = 0; i < src->size; i++) {
        lut->lpid[i] = src->irreg.mlut.gpid[i].lpid;
    }
    MPIR_release_mlut(src->irreg.mlut.t);
    src->irreg.lut.t = lut;
    src->irreg.lut.lpid = src->irreg.lut.t->lpid;
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " avtid %d", src->avtid));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_CHECK_CONVERT_MLUT_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int check_convert_lut_to_regular(MPIR_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS;
    int mode_detected, offset, blocksize, stride;
    MPIR_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_CHECK_CONVERT_LUT_TO_REGULAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_CHECK_CONVERT_LUT_TO_REGULAR);

    if (src->mode != MPIR_RANK_MAP_LUT && src->mode != MPIR_RANK_MAP_LUT_INTRA) {
        goto fn_exit;
    }

    lut = src->irreg.lut.t;
    mode_detected = detect_regular_model(src->irreg.lut.lpid, src->size, &offset, &blocksize,
                                         &stride);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " detected mode: %d", mode_detected));


    switch (mode_detected) {
        case SRC_MAPPER_DIRECT:
            src->mode = MPIR_RANK_MAP_DIRECT;
            if (src->avtid == 0) {
                src->mode = MPIR_RANK_MAP_DIRECT_INTRA;
            }
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIR_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tlut to mode %d",
                                                     (int) src->mode));
            break;
        case SRC_MAPPER_OFFSET:
            src->mode = MPIR_RANK_MAP_OFFSET;
            if (src->avtid == 0) {
                src->mode = MPIR_RANK_MAP_OFFSET_INTRA;
            }
            src->reg.offset = offset;
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIR_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "  lut to mode %d",
                                                     (int) src->mode));
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\toffset: %d",
                                                     src->reg.offset));
            break;
        case SRC_MAPPER_STRIDE:
            if (blocksize == 1) {
                src->mode = MPIR_RANK_MAP_STRIDE;
                if (src->avtid == 0) {
                    src->mode = MPIR_RANK_MAP_STRIDE_INTRA;
                }
            } else {
                src->mode = MPIR_RANK_MAP_STRIDE_BLOCK;
                if (src->avtid == 0) {
                    src->mode = MPIR_RANK_MAP_STRIDE_BLOCK_INTRA;
                }
            }
            src->reg.stride.stride = stride;
            src->reg.stride.blocksize = blocksize;
            src->reg.stride.offset = offset;
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIR_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "  lut to mode %d",
                                                     (int) src->mode));
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\toffset: %d",
                                                     src->reg.stride.offset));
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tblocksize: %d",
                                                     src->reg.stride.blocksize));
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tstride: %d",
                                                     src->reg.stride.stride));
            break;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_CHECK_CONVERT_LUT_TO_REGULAR);
    return mpi_errno;
}

static int set_map(MPIR_rank_map_t * src_rmap, MPIR_rank_map_t * dest_rmap,
                   MPIR_Comm_map_t * mapper, int src_comm_size, int total_mapper_size,
                   int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_SET_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_SET_MAP);

    /* Simplest case: MAP_DUP, exact duplication of src_comm */
    if (mapper->type == MPIR_COMM_MAP_TYPE__DUP && src_comm_size == total_mapper_size) {
        direct_of_src_rmap(src_rmap, dest_rmap, mapper);
        goto fn_exit;
    }
    /* single src_comm, newcomm is smaller than src_comm, only one mapper */
    else if (mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR &&
             mapper->src_mapping_size == total_mapper_size) {
        /* check if new comm has the same mapping as src_comm */
        /* detect src_mapping_offset for direct_to_direct and offset_to_offset */
        int mode_detected, offset = 0, blocksize, stride;
        mode_detected = detect_regular_model(mapper->src_mapping, mapper->src_mapping_size, &offset,
                                             &blocksize, &stride);
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "\tdetected mode: %d",
                                                 mode_detected));

        switch (mode_detected) {
            case SRC_MAPPER_DIRECT:
                direct_of_src_rmap(src_rmap, dest_rmap, mapper);
                break;
            case SRC_MAPPER_OFFSET:
                offset_of_src_rmap(src_rmap, dest_rmap, mapper, offset);
                break;
            case SRC_MAPPER_STRIDE:
                stride_of_src_rmap(src_rmap, dest_rmap, mapper, stride, blocksize, offset);
                break;
            default:
                if (src_rmap->mode == MPIR_RANK_MAP_MLUT) {
                    src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size, mapper_offset);
                } else {
                    src_map_to_lut(src_rmap, dest_rmap, mapper, mapper->src_mapping_size,
                                   mapper_offset);
                }
        }
        goto fn_exit;
    }

    /* more complex case: multiple mappers
     * We always alloc lut (or mlut is src_rmap is mlut). We will check if a
     * lut mapping can be converted to something simpler after all the mapper
     * are processed
     */

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, " multiple mapper"));
    if (mapper->type == MPIR_COMM_MAP_TYPE__DUP) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                        (MPL_DBG_FDEST, " check map_size %d, src_comm_size %d",
                         map_size(*mapper), src_comm_size));
        src_comm_to_mlut(src_rmap, dest_rmap, src_comm_size, total_mapper_size, mapper_offset);
    } else {    /* mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR */
        src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size, mapper_offset);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_SET_MAP);
    return mpi_errno;
}

int MPIR_Comm_create_rank_map(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm_map_t *mapper;
    MPIR_Comm *src_comm;
    int total_mapper_size, mapper_offset;

    if (comm == MPIR_Process.comm_world || comm == MPIR_Process.comm_self) {
        /* COMM_WORLD and COMM_SELF have manually initialized rank map */
        return mpi_errno;
    }

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COMM_CREATE_RANK_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COMM_CREATE_RANK_MAP);

    /* do some sanity checks */
    LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
            MPIR_Assert(mapper->dir == MPIR_COMM_MAP_DIR__L2L ||
                        mapper->dir == MPIR_COMM_MAP_DIR__L2R);
        }

        if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
            MPIR_Assert(mapper->dir == MPIR_COMM_MAP_DIR__L2L ||
                        mapper->dir == MPIR_COMM_MAP_DIR__R2L);
        }
    }

    /* First, handle all the mappers that contribute to the local part
     * of the comm */
    total_mapper_size = 0;
    LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2R || mapper->dir == MPIR_COMM_MAP_DIR__R2R)
            continue;

        total_mapper_size += map_size(*mapper);
    }
    mapper_offset = 0;
    LL_FOREACH(comm->mapper_head, mapper) {
        src_comm = mapper->src_comm;

        if (mapper->dir == MPIR_COMM_MAP_DIR__L2R || mapper->dir == MPIR_COMM_MAP_DIR__R2R)
            continue;

        if (mapper->dir == MPIR_COMM_MAP_DIR__L2L) {
            if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->map, &comm->map, mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                       comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->map, &comm->local_map, mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else if (src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM &&
                       comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->local_map, &comm->map, mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else {    /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->local_map, &comm->local_map,
                        mapper, src_comm->local_size, total_mapper_size, mapper_offset);
            }
        } else {        /* mapper->dir == MPIR_COMM_MAP_DIR__R2L */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);

            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->intra, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->map, &comm->map, mapper,
                        src_comm->remote_size, total_mapper_size, mapper_offset);
            } else {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->inter, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->map, &comm->local_map, mapper,
                        src_comm->remote_size, total_mapper_size, mapper_offset);
            }
        }

        mapper_offset += map_size(*mapper);
    }

    /* Next, handle all the mappers that contribute to the remote part
     * of the comm (only valid for intercomms)
     */
    total_mapper_size = 0;
    LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2L || mapper->dir == MPIR_COMM_MAP_DIR__R2L)
            continue;

        total_mapper_size += map_size(*mapper);
    }
    mapper_offset = 0;
    LL_FOREACH(comm->mapper_head, mapper) {
        src_comm = mapper->src_comm;

        if (mapper->dir == MPIR_COMM_MAP_DIR__L2L || mapper->dir == MPIR_COMM_MAP_DIR__R2L)
            continue;

        MPIR_Assert(comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);

        if (mapper->dir == MPIR_COMM_MAP_DIR__L2R) {
            if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->map, &comm->map, mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else {    /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&src_comm->local_map, &comm->map, mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            }
        } else {        /* mapper->dir == MPIR_COMM_MAP_DIR__R2R */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST,
                             " inter->, R2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                             src_comm->remote_size, total_mapper_size, mapper_offset));
            set_map(&src_comm->map, &comm->map, mapper,
                    src_comm->remote_size, total_mapper_size, mapper_offset);
        }

        mapper_offset += map_size(*mapper);
    }

    /* check before finishing
     * 1. if mlut can be converted to lut: all avtids are the same
     * 2. if lut can be converted to regular modes: direct, offset, and more
     */
    check_convert_mlut_to_lut(&comm->map);
    check_convert_lut_to_regular(&comm->map);
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        check_convert_mlut_to_lut(&comm->local_map);
        check_convert_lut_to_regular(&comm->local_map);
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* setup the lut for the local_comm in the intercomm */
        if (comm->local_comm) {
            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, "\t create local_comm using src_comm"));
            direct_of_src_rmap(&comm->local_map, &comm->local_comm->map, NULL);

            MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                            (MPL_DBG_FDEST, "create local_comm using src_comm"));
        }
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        comm->local_map.mode = MPIR_RANK_MAP_NONE;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COMM_CREATE_RANK_MAP);
    return mpi_errno;
}

void MPIR_Comm_destroy_rank_map(MPIR_Comm * comm)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_COMM_DESTROY_RANK_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_COMM_DESTROY_RANK_MAP);

    if (comm->map.mode == MPIR_RANK_MAP_LUT || comm->map.mode == MPIR_RANK_MAP_LUT_INTRA) {
        MPIR_release_lut(comm->map.irreg.lut.t);
    }
    if (comm->local_map.mode == MPIR_RANK_MAP_LUT
        || comm->local_map.mode == MPIR_RANK_MAP_LUT_INTRA) {
        MPIR_release_lut(comm->local_map.irreg.lut.t);
    }
    if (comm->map.mode == MPIR_RANK_MAP_MLUT) {
        MPIR_release_mlut(comm->map.irreg.mlut.t);
    }
    if (comm->local_map.mode == MPIR_RANK_MAP_MLUT) {
        MPIR_release_mlut(comm->local_map.irreg.mlut.t);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_COMM_DESTROY_RANK_MAP);
}

int MPIR_alloc_lut(MPIR_rank_map_lut_t ** lut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_rank_map_lut_t *new_lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALLOC_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALLOC_LUT);

    new_lut = (MPIR_rank_map_lut_t *) MPL_malloc(sizeof(MPIR_rank_map_lut_t)
                                                 + size * sizeof(MPIR_lpid_t), MPL_MEM_ADDRESS);
    if (new_lut == NULL) {
        *lut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_lut, 1);
    *lut = new_lut;

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "alloc lut %p, size %lu, refcount=%d",
                     new_lut, size * sizeof(MPIR_lpid_t), MPIR_Object_get_ref(new_lut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALLOC_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_release_lut(MPIR_rank_map_lut_t * lut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_RELEASE_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_RELEASE_LUT);

    MPIR_Object_release_ref(lut, &in_use);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "dec ref to lut %p", lut));
    if (!in_use) {
        MPL_free(lut);
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "free lut %p", lut));
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_RELEASE_LUT);
    return mpi_errno;
}

int MPIR_alloc_mlut(MPIR_rank_map_mlut_t ** mlut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_rank_map_mlut_t *new_mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALLOC_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALLOC_MLUT);

    new_mlut = (MPIR_rank_map_mlut_t *) MPL_malloc(sizeof(MPIR_rank_map_mlut_t)
                                                   + size * sizeof(MPIR_gpid_t), MPL_MEM_ADDRESS);
    if (new_mlut == NULL) {
        *mlut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_mlut, 1);
    *mlut = new_mlut;

    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE,
                    (MPL_DBG_FDEST, "alloc mlut %p, size %lu, refcount=%d",
                     new_mlut, size * sizeof(MPIR_gpid_t), MPIR_Object_get_ref(new_mlut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALLOC_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_release_mlut(MPIR_rank_map_mlut_t * mlut)
{
    int mpi_errno = MPI_SUCCESS;
    int in_use = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_RELEASE_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_RELEASE_MLUT);

    MPIR_Object_release_ref(mlut, &in_use);
    MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "dec ref to mlut %p", mlut));
    if (!in_use) {
        MPL_free(mlut);
        MPL_DBG_MSG_FMT(MPIR_DBG_COMM, VERBOSE, (MPL_DBG_FDEST, "free mlut %p", mlut));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_RELEASE_MLUT);
    return mpi_errno;
}

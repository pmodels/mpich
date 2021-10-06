/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "mpidch4r.h"
#include "ch4i_comm.h"

enum MPIDI_src_mapper_models {
    MPIDI_SRC_MAPPER_IRREGULAR = 0,
    MPIDI_SRC_MAPPER_DIRECT = 1,
    MPIDI_SRC_MAPPER_OFFSET = 2,
    MPIDI_SRC_MAPPER_STRIDE = 3
};

static int map_size(MPIR_Comm_map_t map);
static int detect_regular_model(int *lpid, int size, int *offset, int *blocksize, int *stride);
static int src_comm_to_mlut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest, int size,
                            int total_mapper_size, int mapper_offset);
static int src_mlut_to_mlut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                            MPIR_Comm_map_t * mapper, int total_mapper_size, int mapper_offset);
static int src_map_to_lut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest, MPIR_Comm_map_t * mapper,
                          int total_mapper_size, int mapper_offset);
static void direct_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper);
static void offset_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int offset);
static void stride_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int stride, int blocksize, int offset);
static int check_convert_mlut_to_lut(MPIDI_rank_map_t * src);
static int check_convert_lut_to_regular(MPIDI_rank_map_t * src);
static int set_map(MPIDI_rank_map_t * src_rmap, MPIDI_rank_map_t * dest_rmap,
                   MPIR_Comm_map_t * mapper, int src_comm_size, int total_mapper_size,
                   int mapper_offset);

static int map_size(MPIR_Comm_map_t map)
{
    int ret = 0;
    MPIR_FUNC_ENTER;

    if (map.type == MPIR_COMM_MAP_TYPE__IRREGULAR)
        ret = map.src_mapping_size;
    else if (map.dir == MPIR_COMM_MAP_DIR__L2L || map.dir == MPIR_COMM_MAP_DIR__L2R)
        ret = map.src_comm->local_size;
    else
        ret = map.src_comm->remote_size;

    MPIR_FUNC_EXIT;
    return ret;
}

static int detect_regular_model(int *lpid, int size, int *offset, int *blocksize, int *stride)
{
    int off = 0, bs = 0, st = 0;
    int i;
    int ret = MPIDI_SRC_MAPPER_IRREGULAR;

    MPIR_FUNC_ENTER;

    if (size == 0) {
        ret = MPIDI_SRC_MAPPER_DIRECT;
        goto fn_exit;
    }

    off = lpid[0];
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: offset %d", off));

    for (i = 0; i < size; i++) {
        if (lpid[i] != i + off) {
            break;
        }
        bs++;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, "\tdetect model: blocksize %d", bs));
    if (bs == size) {
        if (off == 0) {
            ret = MPIDI_SRC_MAPPER_DIRECT;
            goto fn_exit;
        } else {
            *offset = off;
            ret = MPIDI_SRC_MAPPER_OFFSET;
            goto fn_exit;
        }
    }

    /* blocksize less than total size, try if this is stride */
    st = lpid[bs] - lpid[0];
    if (st < 0 || st <= bs) {
        ret = MPIDI_SRC_MAPPER_IRREGULAR;
        goto fn_exit;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: stride %d", st));
    for (i = bs; i < size; i++) {
        if (lpid[i] != MPIDI_CALC_STRIDE(i, st, bs, off)) {
            ret = MPIDI_SRC_MAPPER_IRREGULAR;
            goto fn_exit;
        }
    }
    *offset = off;
    *blocksize = bs;
    *stride = st;
    ret = MPIDI_SRC_MAPPER_STRIDE;

  fn_exit:
    MPIR_FUNC_EXIT;
    return ret;
}

static int src_comm_to_mlut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest, int size,
                            int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_ENTER;

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_mlut(&mlut, total_mapper_size);
        MPIR_ERR_CHECK(mpi_errno);
        dest->size = total_mapper_size;
        dest->mode = MPIDI_RANK_MAP_MLUT;
        dest->avtid = -1;
        dest->irreg.mlut.t = mlut;
        dest->irreg.mlut.gpid = mlut->gpid;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " size %d", size));
    switch (src->mode) {
        case MPIDI_RANK_MAP_DIRECT:
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = i;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            break;
        case MPIDI_RANK_MAP_OFFSET:
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = i + src->reg.offset;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
            break;
        case MPIDI_RANK_MAP_STRIDE:
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = MPIDI_CALC_STRIDE_SIMPLE(i,
                                                                                         src->reg.
                                                                                         stride.stride,
                                                                                         src->reg.
                                                                                         stride.offset);
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = MPIDI_CALC_STRIDE(i,
                                                                                  src->reg.stride.
                                                                                  stride,
                                                                                  src->reg.stride.
                                                                                  blocksize,
                                                                                  src->reg.stride.
                                                                                  offset);
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[i];
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            }
            break;
        case MPIDI_RANK_MAP_MLUT:
            for (i = 0; i < size; i++) {
                dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[i].lpid;
                dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[i].avtid;
            }
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int src_mlut_to_mlut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                            MPIR_Comm_map_t * mapper, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = map_size(*mapper);
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_ENTER;

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_mlut(&mlut, total_mapper_size);
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
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " src mode %d, dest mode %d",
                     (int) src->mode, (int) dest->mode));
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int src_map_to_lut(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest, MPIR_Comm_map_t * mapper,
                          int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = map_size(*mapper);
    MPIDI_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_ENTER;

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_lut(&lut, total_mapper_size);
        MPIR_ERR_CHECK(mpi_errno);
        dest->size = total_mapper_size;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " size %d, mapper->src_mapping_size %d",
                     size, mapper->src_mapping_size));
    dest->mode = MPIDI_RANK_MAP_LUT;
    dest->avtid = src->avtid;
    dest->irreg.lut.t = lut;
    dest->irreg.lut.lpid = lut->lpid;
    switch (src->mode) {
        case MPIDI_RANK_MAP_DIRECT:
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i];
            }
            break;
        case MPIDI_RANK_MAP_OFFSET:
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i] + src->reg.offset;
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
            break;
        case MPIDI_RANK_MAP_STRIDE:
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] =
                    MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i], src->reg.stride.stride,
                                             src->reg.stride.offset);
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] = MPIDI_CALC_STRIDE(mapper->src_mapping[i],
                                                                            src->reg.stride.stride,
                                                                            src->reg.
                                                                            stride.blocksize,
                                                                            src->reg.stride.offset);
            }
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                             src->reg.stride.stride, src->reg.stride.blocksize,
                             src->reg.stride.offset));
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            for (i = 0; i < size; i++) {
                dest->irreg.lut.lpid[i + mapper_offset] =
                    src->irreg.lut.lpid[mapper->src_mapping[i]];
            }
            break;
        default:
            mpi_errno = 1;
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, " cannot convert mode %d to lut", (int) src->mode));
            goto fn_fail;
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static void direct_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper)
{
    MPIR_FUNC_ENTER;
    dest->mode = src->mode;
    if (mapper) {
        dest->size = map_size(*mapper);
    } else {
        dest->size = src->size;
    }
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIDI_RANK_MAP_DIRECT:
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            break;
        case MPIDI_RANK_MAP_OFFSET:
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            dest->reg.offset = src->reg.offset;
            break;
        case MPIDI_RANK_MAP_STRIDE:
        case MPIDI_RANK_MAP_STRIDE_INTRA:
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset;
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            dest->irreg.lut.t = src->irreg.lut.t;
            dest->irreg.lut.lpid = src->irreg.lut.lpid;
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tref count %d",
                             MPIR_Object_get_ref(src->irreg.lut.t)));
            MPIR_Object_add_ref(src->irreg.lut.t);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
            break;
        case MPIDI_RANK_MAP_MLUT:
            dest->irreg.mlut.t = src->irreg.mlut.t;
            dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
            MPIR_Object_add_ref(src->irreg.mlut.t);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_EXIT;
}

static void offset_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int offset)
{
    MPIR_FUNC_ENTER;
    dest->avtid = src->avtid;
    dest->size = map_size(*mapper);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            dest->mode = MPIDI_RANK_MAP_OFFSET_INTRA;
            dest->reg.offset = offset;
            break;
        case MPIDI_RANK_MAP_DIRECT:
            dest->mode = MPIDI_RANK_MAP_OFFSET;
            dest->reg.offset = offset;
            break;
        case MPIDI_RANK_MAP_OFFSET:
            dest->mode = MPIDI_RANK_MAP_OFFSET;
            dest->reg.offset = src->reg.offset + offset;
            break;
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            dest->mode = MPIDI_RANK_MAP_OFFSET_INTRA;
            dest->reg.offset = src->reg.offset + offset;
            break;
        case MPIDI_RANK_MAP_STRIDE:
            dest->mode = MPIDI_RANK_MAP_STRIDE;
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
            break;
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            dest->mode = MPIDI_RANK_MAP_STRIDE_INTRA;
            dest->reg.stride.stride = src->reg.stride.stride;
            dest->reg.stride.blocksize = src->reg.stride.blocksize;
            dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            dest->mode = src->mode;
            dest->irreg.lut.t = src->irreg.lut.t;
            dest->irreg.lut.lpid = &src->irreg.lut.lpid[offset];
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tref count %d",
                             MPIR_Object_get_ref(src->irreg.lut.t)));
            MPIR_Object_add_ref(src->irreg.lut.t);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
            break;
        case MPIDI_RANK_MAP_MLUT:
            dest->mode = src->mode;
            dest->irreg.mlut.t = src->irreg.mlut.t;
            dest->irreg.mlut.gpid = &src->irreg.mlut.gpid[offset];
            MPIR_Object_add_ref(src->irreg.mlut.t);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_EXIT;
}

static void stride_of_src_rmap(MPIDI_rank_map_t * src, MPIDI_rank_map_t * dest,
                               MPIR_Comm_map_t * mapper, int stride, int blocksize, int offset)
{
    MPIR_FUNC_ENTER;
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
        case MPIDI_RANK_MAP_DIRECT_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE_INTRA;
            } else {
                dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset;
            MPIR_Assert(stride > 0);
            MPIR_Assert(blocksize > 0);
            break;
        case MPIDI_RANK_MAP_DIRECT:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE;
            } else {
                dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset;
            MPIR_Assert(stride > 0);
            MPIR_Assert(blocksize > 0);
            break;
        case MPIDI_RANK_MAP_OFFSET:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE;
            } else {
                dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset + src->reg.offset;
            break;
        case MPIDI_RANK_MAP_OFFSET_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE_INTRA;
            } else {
                dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA;
            }
            dest->size = map_size(*mapper);
            dest->reg.stride.stride = stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = offset + src->reg.offset;
            break;
        case MPIDI_RANK_MAP_STRIDE:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE;
                dest->reg.stride.stride = src->reg.stride.stride * stride;
                dest->reg.stride.blocksize = blocksize;
                dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
            } else {
                src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            }
            break;
        case MPIDI_RANK_MAP_STRIDE_INTRA:
            if (blocksize == 1) {
                dest->mode = MPIDI_RANK_MAP_STRIDE_INTRA;
                dest->reg.stride.stride = src->reg.stride.stride * stride;
                dest->reg.stride.blocksize = blocksize;
                dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
            } else {
                src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            }
            break;
        case MPIDI_RANK_MAP_STRIDE_BLOCK:
        case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIDI_RANK_MAP_LUT:
        case MPIDI_RANK_MAP_LUT_INTRA:
            src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIDI_RANK_MAP_MLUT:
            src_mlut_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
            break;
        case MPIDI_RANK_MAP_NONE:
            MPIR_Assert(0);
            break;
    }
    MPIR_FUNC_EXIT;
}

static int check_convert_mlut_to_lut(MPIDI_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS, i;
    int flag = 1;
    int avtid;
    MPIDI_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_ENTER;

    if (src->mode != MPIDI_RANK_MAP_MLUT) {
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
        src->mode = MPIDI_RANK_MAP_LUT_INTRA;
    } else {
        src->mode = MPIDI_RANK_MAP_LUT;
    }
    mpi_errno = MPIDIU_alloc_lut(&lut, src->size);
    MPIR_ERR_CHECK(mpi_errno);
    for (i = 0; i < src->size; i++) {
        lut->lpid[i] = src->irreg.mlut.gpid[i].lpid;
    }
    MPIDIU_release_mlut(src->irreg.mlut.t);
    src->irreg.lut.t = lut;
    src->irreg.lut.lpid = src->irreg.lut.t->lpid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " avtid %d", src->avtid));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int check_convert_lut_to_regular(MPIDI_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS;
    int mode_detected, offset, blocksize, stride;
    MPIDI_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_ENTER;

    if (src->mode != MPIDI_RANK_MAP_LUT && src->mode != MPIDI_RANK_MAP_LUT_INTRA) {
        goto fn_exit;
    }

    lut = src->irreg.lut.t;
    mode_detected = detect_regular_model(src->irreg.lut.lpid, src->size, &offset, &blocksize,
                                         &stride);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " detected mode: %d", mode_detected));


    switch (mode_detected) {
        case MPIDI_SRC_MAPPER_DIRECT:
            src->mode = MPIDI_RANK_MAP_DIRECT;
            if (src->avtid == 0) {
                src->mode = MPIDI_RANK_MAP_DIRECT_INTRA;
            }
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIDIU_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tlut to mode %d", (int) src->mode));
            break;
        case MPIDI_SRC_MAPPER_OFFSET:
            src->mode = MPIDI_RANK_MAP_OFFSET;
            if (src->avtid == 0) {
                src->mode = MPIDI_RANK_MAP_OFFSET_INTRA;
            }
            src->reg.offset = offset;
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIDIU_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "  lut to mode %d", (int) src->mode));
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\toffset: %d", src->reg.offset));
            break;
        case MPIDI_SRC_MAPPER_STRIDE:
            if (blocksize == 1) {
                src->mode = MPIDI_RANK_MAP_STRIDE;
                if (src->avtid == 0) {
                    src->mode = MPIDI_RANK_MAP_STRIDE_INTRA;
                }
            } else {
                src->mode = MPIDI_RANK_MAP_STRIDE_BLOCK;
                if (src->avtid == 0) {
                    src->mode = MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA;
                }
            }
            src->reg.stride.stride = stride;
            src->reg.stride.blocksize = blocksize;
            src->reg.stride.offset = offset;
            src->irreg.lut.t = NULL;
            src->irreg.lut.lpid = NULL;
            MPIDIU_release_lut(lut);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "  lut to mode %d", (int) src->mode));
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\toffset: %d", src->reg.stride.offset));
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tblocksize: %d", src->reg.stride.blocksize));
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\tstride: %d", src->reg.stride.stride));
            break;
    }
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int set_map(MPIDI_rank_map_t * src_rmap, MPIDI_rank_map_t * dest_rmap,
                   MPIR_Comm_map_t * mapper, int src_comm_size, int total_mapper_size,
                   int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

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
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tdetected mode: %d", mode_detected));

        switch (mode_detected) {
            case MPIDI_SRC_MAPPER_DIRECT:
                direct_of_src_rmap(src_rmap, dest_rmap, mapper);
                break;
            case MPIDI_SRC_MAPPER_OFFSET:
                offset_of_src_rmap(src_rmap, dest_rmap, mapper, offset);
                break;
            case MPIDI_SRC_MAPPER_STRIDE:
                stride_of_src_rmap(src_rmap, dest_rmap, mapper, stride, blocksize, offset);
                break;
            default:
                if (src_rmap->mode == MPIDI_RANK_MAP_MLUT) {
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

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " multiple mapper"));
    if (mapper->type == MPIR_COMM_MAP_TYPE__DUP) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " check map_size %d, src_comm_size %d",
                         map_size(*mapper), src_comm_size));
        src_comm_to_mlut(src_rmap, dest_rmap, src_comm_size, total_mapper_size, mapper_offset);
    } else {    /* mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR */
        src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size, mapper_offset);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_comm_create_rank_map(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm_map_t *mapper;
    MPIR_Comm *src_comm;
    int total_mapper_size, mapper_offset;


    MPIR_FUNC_ENTER;

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
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                       comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, local_map), mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else if (src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM &&
                       comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, map), mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else {    /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, local_map),
                        mapper, src_comm->local_size, total_mapper_size, mapper_offset);
            }
        } else {        /* mapper->dir == MPIR_COMM_MAP_DIR__R2L */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);

            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->intra, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                        src_comm->remote_size, total_mapper_size, mapper_offset);
            } else {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->inter, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, local_map), mapper,
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
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            } else {    /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, map), mapper,
                        src_comm->local_size, total_mapper_size, mapper_offset);
            }
        } else {        /* mapper->dir == MPIR_COMM_MAP_DIR__R2R */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST,
                             " inter->, R2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                             src_comm->remote_size, total_mapper_size, mapper_offset));
            set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                    src_comm->remote_size, total_mapper_size, mapper_offset);
        }

        mapper_offset += map_size(*mapper);
    }

    /* check before finishing
     * 1. if mlut can be converted to lut: all avtids are the same
     * 2. if lut can be converted to regular modes: direct, offset, and more
     */
    check_convert_mlut_to_lut(&MPIDI_COMM(comm, map));
    check_convert_lut_to_regular(&MPIDI_COMM(comm, map));
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        check_convert_mlut_to_lut(&MPIDI_COMM(comm, local_map));
        check_convert_lut_to_regular(&MPIDI_COMM(comm, local_map));
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* setup the lut for the local_comm in the intercomm */
        if (comm->local_comm) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\t create local_comm using src_comm"));
            direct_of_src_rmap(&MPIDI_COMM(comm, local_map),
                               &MPIDI_COMM(comm->local_comm, map), NULL);

            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                            (MPL_DBG_FDEST, "create local_comm using src_comm"));
        }
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDI_COMM(comm, local_map).mode = MPIDI_RANK_MAP_NONE;
    }
#ifdef MPL_USE_DBG_LOGGING
    int rank_;
    int avtid_, lpid_ = -1;
    if (comm->remote_size < 16) {
        for (rank_ = 0; rank_ < comm->remote_size; ++rank_) {
            MPIDIU_comm_rank_to_pid(comm, rank_, &lpid_, &avtid_);
            MPIDIU_comm_rank_to_av(comm, rank_);
        }
    }
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm->local_size < 16) {
        for (rank_ = 0; rank_ < comm->local_size; ++rank_) {
            MPIDIU_comm_rank_to_pid_local(comm, rank_, &lpid_, &avtid_);
        }
    }
#endif

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* number of leading zeros, from Hacker's Delight */
static int nlz(uint32_t x)
{
    uint32_t y;
    int n = 32;
/* *INDENT-OFF* */
    y = x >> 16; if (y != 0) { n = n - 16; x = y; }
    y = x >> 8; if (y != 0) { n = n - 8; x = y; }
    y = x >> 4; if (y != 0) { n = n - 4; x = y; }
    y = x >> 2; if (y != 0) { n = n - 2; x = y; }
    y = x >> 1; if (y != 0) { return n - 2; }
/* *INDENT-ON* */
    return n - x;
}

/* 0xab00000cde -> 0xabcde if num_low_bits is 12 */
static uint64_t shrink(uint64_t x, int num_low_bits)
{
    return ((x >> 32) << num_low_bits) + (x & 0xffffffff);
}

int MPIDI_check_disjoint_gpids(uint64_t gpids1[], int n1, uint64_t gpids2[], int n2)
{
    int mpi_errno = MPI_SUCCESS;
    uint32_t gpidmaskPrealloc[128];
    uint32_t *gpidmask;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_ENTER;

    /* Taking the knowledge that gpid are two 32-bit avtid + lpid, both are
     * often in small range. If we shrink the middle gaps between avtid and
     * lpid, the number shouldn't be too large. */

    /* Find the max low-32-bit gpid */
    uint64_t max_lpid = 0;
    for (int i = 0; i < n1; i++) {
        uint64_t n = gpids1[i] & 0xffffffff;
        if (n > max_lpid)
            max_lpid = n;
    }
    for (int i = 0; i < n2; i++) {
        uint64_t n = gpids2[i] & 0xffffffff;
        if (n > max_lpid)
            max_lpid = n;
    }

    int num_low_bits = 32 - nlz((uint32_t) max_lpid);

    uint64_t max_gpid = 0;
    for (int i = 0; i < n1; i++) {
        uint64_t n = shrink(gpids1[i], num_low_bits);
        if (n > max_gpid)
            max_gpid = n;
    }
    for (int i = 0; i < n2; i++) {
        uint64_t n = shrink(gpids2[i], num_low_bits);
        if (n > max_gpid)
            max_gpid = n;
    }

    uint64_t mask_size = (max_gpid / 32) + 1;
    if (mask_size > 128) {
        MPIR_CHKLMEM_MALLOC(gpidmask, uint32_t *, mask_size * sizeof(uint32_t),
                            mpi_errno, "gpidmask", MPL_MEM_COMM);
    } else {
        gpidmask = gpidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(gpidmask, 0x00, mask_size * sizeof(*gpidmask));

    /* Set the bits for the first array */
    for (int i = 0; i < n1; i++) {
        uint64_t n = shrink(gpids1[i], num_low_bits);
        int idx = n / 32;
        int bit = n % 32;
        gpidmask[idx] = gpidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (int i = 0; i < n2; i++) {
        uint64_t n = shrink(gpids2[i], num_low_bits);
        int idx = n / 32;
        int bit = n % 32;
        if (gpidmask[idx] & (1 << bit)) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", gpids2[i]);
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        gpidmask[idx] = gpidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

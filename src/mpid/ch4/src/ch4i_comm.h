/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef CH4I_COMM_H_INCLUDED
#define CH4I_COMM_H_INCLUDED

#include "ch4_types.h"
#include "mpl_utlist.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_alloc_md_stride_params
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_alloc_md_stride_params(MPIDI_md_stride_params_t ** md_stride_params, int dims)
{
    int mpi_errno = MPI_SUCCESS, alloc_dims = 3 * dims;
    MPIDI_md_stride_params_t *new_md_stride_params = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLOC_MD_STRIDE_PARAMS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLOC_MD_STRIDE_PARAMS);

    new_md_stride_params = (MPIDI_md_stride_params_t *) MPL_malloc(sizeof(MPIDI_md_stride_params_t)
                                                                   + (3 * alloc_dims) * sizeof(int));
    if (new_md_stride_params == NULL) {
        *md_stride_params = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_md_stride_params, 1);
    *md_stride_params = new_md_stride_params;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc md_stride_params %p, size %ld, refcount=%d",
                     new_md_stride_params, 3 * alloc_dims * sizeof(int), MPIR_Object_get_ref(new_md_stride_params)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLOC_MD_STRIDE_PARAMS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_release_md_stride_params
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_release_md_stride_params(MPIDI_md_stride_params_t * md_stride_params)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_MD_STRIDE_PARAMS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_MD_STRIDE_PARAMS);

    MPIR_Object_release_ref(md_stride_params, &count);
    if (count == 0) {
        MPL_free(md_stride_params);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                        (MPL_DBG_FDEST, "free md_stride_params %p", md_stride_params));
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_MD_STRIDE_PARAMS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_map_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_map_size(MPIR_Comm_map_t map)
{
    int ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_MAP_SIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_MAP_SIZE);

    if (map.type == MPIR_COMM_MAP_TYPE__IRREGULAR)
        ret = map.src_mapping_size;
    else if (map.dir == MPIR_COMM_MAP_DIR__L2L || map.dir == MPIR_COMM_MAP_DIR__L2R)
        ret = map.src_comm->local_size;
    else
        ret = map.src_comm->remote_size;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_MAP_SIZE);
    return ret;
}

/*
 * This enum is used exclusively in this header file
 */
enum MPIDI_src_mapper_models {
    MPIDI_SRC_MAPPER_IRREGULAR = 0,
    MPIDI_SRC_MAPPER_DIRECT = 1,
    MPIDI_SRC_MAPPER_OFFSET = 2,
    MPIDI_SRC_MAPPER_STRIDE = 3,
    MPIDI_SRC_MAPPER_MD_STRIDE = 4
};

#undef FUNCNAME
#define FUNCNAME MPIDI_calc_md_stride
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_calc_md_stride(int rank, int dim, int *params)
{
    int ld_rank;
    if (dim == 0) {
        return MPIDI_CALC_STRIDE(rank, params[0], params[1], params[2]);
    }

    ld_rank = rank - rank / params[1] * params[1];
    return MPIDI_CALC_STRIDE(rank, params[0], params[1], params[2])
           + MPIDI_calc_md_stride(ld_rank, dim - 1, params - 3);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_detect_stride
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_detect_md_stride(int *lpid, int size,
                                         int stride, int blocksize, int offset)
{
    int i, ret = MPIDI_SRC_MAPPER_IRREGULAR;
    int off = 0, bs = 0, st = 0;
    int dim_curr = 0;
    int *params_p = MPIDI_md_stride_params;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DETECT_MD_STRIDE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DETECT_MD_STRIDE);

    params_p[0] = stride;
    params_p[1] = blocksize;
    params_p[2] = offset;

    while (dim_curr < MPIDI_MD_STRIDE_DIM_MAX) {
        for (i = 0; i < size; i++) {
            if (lpid[i] != MPIDI_calc_md_stride(i, dim_curr, params_p)) {
                dim_curr += 1;
                if (dim_curr >= MPIDI_MD_STRIDE_DIM_MAX)
                    goto fn_exit;
                params_p += 3;
                params_p[0] = lpid[i] - lpid[0];
                params_p[1] = i;
                params_p[2] = 0;
                break;
            }
        }
        if (i == size) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST,
                             "\tdetect model: md_stride dim=%d", dim_curr));
            ret = MPIDI_SRC_MAPPER_MD_STRIDE;
            goto fn_exit;
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DETECT_MD_STRIDE);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_detect_regular_model
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_detect_regular_model(int *lpid, int size,
                                             int *offset, int *blocksize, int *stride)
{
    int off = 0, bs = 0, st = 0;
    int i;
    int ret = MPIDI_SRC_MAPPER_IRREGULAR;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DETECT_REGULAR_MODEL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DETECT_REGULAR_MODEL);

    off = lpid[0];
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, "\tdetect model: offset %d", off));

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
        }
        else {
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
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, "\tdetect model: stride %d", st));
    for (i = bs; i < size; i++) {
        if (lpid[i] != MPIDI_CALC_STRIDE(i, st, bs, off)) {
            ret = MPIDI_detect_md_stride(lpid, size, st, bs, off);
            goto fn_exit;
        }
    }
    *offset = off;
    *blocksize = bs;
    *stride = st;
    ret = MPIDI_SRC_MAPPER_STRIDE;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DETECT_REGULAR_MODEL);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_src_comm_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_src_comm_to_lut(MPIDI_rank_map_t * src,
                                        MPIDI_rank_map_t * dest,
                                        int size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIDI_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SRC_COMM_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SRC_COMM_TO_LUT);

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_lut(&lut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
        dest->mode = MPIDI_RANK_MAP_LUT;
        dest->avtid = src->avtid;
        dest->irreg.lut.t = lut;
        dest->irreg.lut.lpid = lut->lpid;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " size %d", size));
    switch (src->mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = i;
        }
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = i + src->reg.offset;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = MPIDI_CALC_STRIDE_SIMPLE(i,
                                                                               src->reg.stride.
                                                                               stride,
                                                                               src->reg.stride.
                                                                               offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = MPIDI_CALC_STRIDE(i,
                                                                        src->reg.stride.stride,
                                                                        src->reg.stride.blocksize,
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
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[i];
        }
        break;
    case MPIDI_RANK_MAP_MLUT:
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SRC_COMM_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_src_comm_to_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_src_comm_to_mlut(MPIDI_rank_map_t * src,
                                         MPIDI_rank_map_t * dest,
                                         int size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i, _i; /* _i only used with MLUT */
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SRC_COMM_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SRC_COMM_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_mlut(&mlut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                MPIDI_CALC_STRIDE_SIMPLE(i,
                                         src->reg.stride.stride,
                                         src->reg.stride.offset);
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
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                MPIDI_CALC_STRIDE(i,
                                  src->reg.stride.stride,
                                  src->reg.stride.blocksize,
                                  src->reg.stride.offset);
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                MPIDI_calc_md_stride(i,
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params);
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source md_stride dim %d params %p",
                         src->reg.md_stride.dims, src->reg.md_stride.params));
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[i];
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE_SIMPLE(i,
                                         src->reg.stride.stride,
                                         src->reg.stride.offset)];
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE(i,
                                  src->reg.stride.stride,
                                  src->reg.stride.blocksize,
                                  src->reg.stride.offset)];
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_calc_md_stride(i,
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params)];
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[i].lpid;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[i].avtid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
        _i = MPIDI_CALC_STRIDE_SIMPLE(i,
                                      src->reg.stride.stride,
                                      src->reg.stride.offset);
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[_i].lpid;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[_i].avtid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
        _i = MPIDI_CALC_STRIDE(i,
                               src->reg.stride.stride,
                               src->reg.stride.blocksize,
                               src->reg.stride.offset);
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[_i].lpid;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[_i].avtid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        _i = MPIDI_calc_md_stride(i,
                                  src->reg.md_stride.dims,
                                  src->reg.md_stride.params);
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[_i].lpid;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[_i].avtid;
        }
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SRC_COMM_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_src_mlut_to_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_src_mlut_to_mlut(MPIDI_rank_map_t * src,
                                         MPIDI_rank_map_t * dest,
                                         MPIR_Comm_map_t * mapper,
                                         int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i, _i;
    int size = MPIDI_map_size(*mapper);
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SRC_MLUT_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SRC_MLUT_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_mlut(&mlut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
    }

    dest->mode = MPIDI_RANK_MAP_MLUT;
    dest->irreg.mlut.t = mlut;
    dest->irreg.mlut.gpid = mlut->gpid;
    switch (src->mode) {
    case MPIDI_RANK_MAP_MLUT:
        _i = mapper->src_mapping[i];
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
        _i = MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i],
                                      src->reg.stride.stride,
                                      src->reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
        _i = MPIDI_CALC_STRIDE(mapper->src_mapping[i],
                               src->reg.stride.stride,
                               src->reg.stride.blocksize,
                               src->reg.stride.offset);
        break;
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        _i = MPIDI_calc_md_stride(mapper->src_mapping[i],
                                  src->reg.md_stride.dims,
                                  src->reg.md_stride.params);
        break;
    default:
        MPIR_Assert(0);
        break;
    }
    for (i = 0; i < size; i++) {
        dest->irreg.mlut.gpid[i + mapper_offset].avtid =
            src->irreg.mlut.gpid[_i].avtid;
        dest->irreg.mlut.gpid[i + mapper_offset].lpid =
            src->irreg.mlut.gpid[_i].lpid;
    }

  fn_exit:
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " src mode %d, dest mode %d",
                     (int) src->mode, (int) dest->mode));
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SRC_MLUT_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_src_map_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_src_map_to_lut(MPIDI_rank_map_t * src,
                                       MPIDI_rank_map_t * dest,
                                       MPIR_Comm_map_t * mapper,
                                       int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = MPIDI_map_size(*mapper);
    MPIDI_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SRC_MAP_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SRC_MAP_TO_LUT);

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_lut(&lut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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
                                                                        src->reg.stride.blocksize,
                                                                        src->reg.stride.offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] =
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source md_stride %d params %p",
                         src->reg.md_stride.dims, src->reg.md_stride.params));
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[mapper->src_mapping[i]];
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i],
                                         src->reg.stride.stride,
                                         src->reg.stride.offset)];
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE(mapper->src_mapping[i],
                                  src->reg.stride.stride,
                                  src->reg.stride.blocksize,
                                  src->reg.stride.offset)];
        }
        break;
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params)];
        }
        break;
    default:
        mpi_errno = 1;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " cannot convert mode %d to lut", (int) src->mode));
        goto fn_fail;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SRC_MAP_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_src_map_to_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_src_map_to_mlut(MPIDI_rank_map_t * src,
                                        MPIDI_rank_map_t * dest,
                                        MPIR_Comm_map_t * mapper,
                                        int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = MPIDI_map_size(*mapper);
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SRC_MAP_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SRC_MAP_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIDIU_alloc_mlut(&mlut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " size %d, mapper->src_mapping_size %d",
                     size, mapper->src_mapping_size));
    dest->mode = MPIDI_RANK_MAP_MLUT;
    dest->avtid = -1;
    dest->irreg.mlut.t = mlut;
    dest->irreg.mlut.gpid = mlut->gpid;
    switch (src->mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = mapper->src_mapping[i];
        }
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                mapper->src_mapping[i] + src->reg.offset;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
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
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = 
                MPIDI_CALC_STRIDE(mapper->src_mapping[i], src->reg.stride.stride,
                                  src->reg.stride.blocksize, src->reg.stride.offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source md_stride %d params %p",
                         src->reg.md_stride.dims, src->reg.md_stride.params));
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                src->irreg.lut.lpid[mapper->src_mapping[i]];
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i],
                                         src->reg.stride.stride,
                                         src->reg.stride.offset)];
        }
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_CALC_STRIDE(mapper->src_mapping[i], src->reg.stride.stride,
                                  src->reg.stride.blocksize, src->reg.stride.offset)];
        }
        break;
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params)];
        }
        break;
    case MPIDI_RANK_MAP_MLUT:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid =
                src->irreg.mlut.gpid[mapper->src_mapping[i]].avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid =
                src->irreg.mlut.gpid[mapper->src_mapping[i]].lpid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[
                MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i],
                                         src->reg.stride.stride,
                                         src->reg.stride.offset)].avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[
                MPIDI_CALC_STRIDE_SIMPLE(mapper->src_mapping[i],
                                         src->reg.stride.stride,
                                         src->reg.stride.offset)].lpid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[
                MPIDI_CALC_STRIDE(mapper->src_mapping[i], src->reg.stride.stride,
                                  src->reg.stride.blocksize, src->reg.stride.offset)].avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[
                MPIDI_CALC_STRIDE(mapper->src_mapping[i], src->reg.stride.stride,
                                  src->reg.stride.blocksize, src->reg.stride.offset)].lpid;
        }
        break;
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params)].avtid;
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[
                MPIDI_calc_md_stride(mapper->src_mapping[i],
                                     src->reg.md_stride.dims,
                                     src->reg.md_stride.params)].lpid;
        }
        break;
    default:
        mpi_errno = 1;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " cannot convert mode %d to mlut", (int) src->mode));
        goto fn_fail;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SRC_MAP_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_direct_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_direct_of_src_rmap(MPIDI_rank_map_t * src,
                                            MPIDI_rank_map_t * dest, MPIR_Comm_map_t * mapper)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_DIRECT_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_DIRECT_OF_SRC_RMAP);
    dest->mode = src->mode;
    if (mapper) {
        dest->size = MPIDI_map_size(*mapper);
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
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        dest->reg.md_stride.dims = src->reg.md_stride.dims;
        dest->reg.md_stride.params = src->reg.md_stride.params;
        dest->reg.md_stride.params_p = src->reg.md_stride.params_p;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d",
                         MPIR_Object_get_ref(src->reg.md_stride.params_p)));
        MPIR_Object_add_ref(src->reg.md_stride.params_p);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tadd ref to src md_stride params_p"));
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = src->irreg.lut.lpid;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset;
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = src->irreg.lut.lpid;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        dest->reg.md_stride.dims = src->reg.md_stride.dims;
        dest->reg.md_stride.params = src->reg.md_stride.params;
        dest->reg.md_stride.params_p = src->reg.md_stride.params_p;
        MPIR_Object_add_ref(src->reg.md_stride.params_p);
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = src->irreg.lut.lpid;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDI_RANK_MAP_MLUT:
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset;
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        dest->reg.md_stride.dims = src->reg.md_stride.dims;
        dest->reg.md_stride.params = src->reg.md_stride.params;
        dest->reg.md_stride.params_p = src->reg.md_stride.params_p;
        MPIR_Object_add_ref(src->reg.md_stride.params_p);
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_DIRECT_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_offset_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_offset_of_src_rmap(MPIDI_rank_map_t * src,
                                            MPIDI_rank_map_t * dest,
                                            MPIR_Comm_map_t * mapper, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFFSET_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFFSET_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    dest->size = MPIDI_map_size(*mapper);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        dest->mode = MPIDI_RANK_MAP_OFFSET | (src->mode & 0x1);
        dest->reg.offset = offset;
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        dest->mode = MPIDI_RANK_MAP_OFFSET | (src->mode & 0x1);
        dest->reg.offset = src->reg.offset + offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        dest->mode = MPIDI_RANK_MAP_STRIDE | (src->mode & 0x1);
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
        break;
    /* FIXME: possible improvement is adding a "rank offset" for the stride block */
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        dest->mode = src->mode;
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = &src->irreg.lut.lpid[offset];
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_MLUT:
        dest->mode = src->mode;
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = &src->irreg.mlut.gpid[offset];
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        MPIDI_src_map_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFFSET_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_stride_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_stride_of_src_rmap(MPIDI_rank_map_t * src,
                                            MPIDI_rank_map_t * dest,
                                            MPIR_Comm_map_t * mapper,
                                            int stride, int blocksize, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STRIDE_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STRIDE_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDI_RANK_MAP_STRIDE | (src->mode & 0x1);
        }
        else {
            dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK | (src->mode & 0x1);
        }
        dest->size = MPIDI_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset;
        MPIR_Assert(stride > 0);
        MPIR_Assert(blocksize > 0);
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDI_RANK_MAP_STRIDE | (src->mode & 0x1);
        }
        else {
            dest->mode = MPIDI_RANK_MAP_STRIDE_BLOCK | (src->mode & 0x1);
        }
        dest->size = MPIDI_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset + src->reg.offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDI_RANK_MAP_STRIDE | (src->mode & 0x1);
            dest->reg.stride.stride = src->reg.stride.stride * stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
        }
        else {
            MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        }
        break;
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
        MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDI_RANK_MAP_LUT_STRIDE | (src->mode & 0x1);
        }
        else {
            dest->mode = MPIDI_RANK_MAP_LUT_STRIDE_BLOCK | (src->mode & 0x1);
        }
        dest->size = MPIDI_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset;
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = src->irreg.lut.lpid;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_MLUT:
        if (blocksize == 1) {
            dest->mode = MPIDI_RANK_MAP_MLUT_STRIDE;
        }
        else {
            dest->mode = MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK;
        }
        dest->size = MPIDI_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset;
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        MPIDI_src_mlut_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STRIDE_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_md_stride_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDI_md_stride_of_src_rmap(MPIDI_rank_map_t * src,
                                               MPIDI_rank_map_t * dest,
                                               MPIR_Comm_map_t * mapper)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_MD_STRIDE_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_MD_STRIDE_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    dest->size = MPIDI_map_size(*mapper);
    switch (src->mode) {
    case MPIDI_RANK_MAP_DIRECT:
    case MPIDI_RANK_MAP_DIRECT_INTRA:
        dest->mode = MPIDI_RANK_MAP_MD_STRIDE_BLOCK | (src->mode & 0x1);
        dest->reg.md_stride.dims = MPIDI_md_stride_dim;
        mpi_errno = MPIDI_alloc_md_stride_params(&dest->reg.md_stride.params_p,
                                                 dest->reg.md_stride.dims);
        dest->reg.md_stride.params =
            dest->reg.md_stride.params_p->params + dest->reg.md_stride.dims * 3;
        memcpy(dest->reg.md_stride.params, MPIDI_md_stride_params,
               (3 * dest->reg.md_stride.dims) * sizeof(int));
        break;
    case MPIDI_RANK_MAP_OFFSET:
    case MPIDI_RANK_MAP_OFFSET_INTRA:
        dest->mode = MPIDI_RANK_MAP_MD_STRIDE_BLOCK | (src->mode & 0x1);
        dest->reg.md_stride.dims = MPIDI_md_stride_dim;
        mpi_errno = MPIDI_alloc_md_stride_params(&dest->reg.md_stride.params_p,
                                                 dest->reg.md_stride.dims);
        dest->reg.md_stride.params =
            dest->reg.md_stride.params_p->params + dest->reg.md_stride.dims * 3;
        memcpy(dest->reg.md_stride.params, MPIDI_md_stride_params,
               (3 * dest->reg.md_stride.dims) * sizeof(int));
        dest->reg.md_stride.params[2] += src->reg.offset;
        break;
    case MPIDI_RANK_MAP_STRIDE:
    case MPIDI_RANK_MAP_STRIDE_INTRA:
    case MPIDI_RANK_MAP_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_LUT:
    case MPIDI_RANK_MAP_LUT_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        MPIDI_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_MLUT:
    case MPIDI_RANK_MAP_MLUT_STRIDE:
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        MPIDI_src_mlut_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDI_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_MD_STRIDE_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_check_convert_mlut_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_check_convert_mlut_to_lut(MPIDI_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS, i;
    int flag = 1;
    int avtid;
    MPIDI_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_CONVERT_MLUT_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_CONVERT_MLUT_TO_LUT);

    if (src->mode < MPIDI_RANK_MAP_MLUT) {
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
    switch (src->mode) {
    case MPIDI_RANK_MAP_MLUT:
        if (avtid == 0)
            src->mode = MPIDI_RANK_MAP_LUT_INTRA;
        else
            src->mode = MPIDI_RANK_MAP_LUT;
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE:
        if (avtid == 0)
            src->mode = MPIDI_RANK_MAP_LUT_STRIDE_INTRA;
        else
            src->mode = MPIDI_RANK_MAP_LUT_STRIDE;
        break;
    case MPIDI_RANK_MAP_MLUT_STRIDE_BLOCK:
        if (avtid == 0)
            src->mode = MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA;
        else
            src->mode = MPIDI_RANK_MAP_LUT_STRIDE_BLOCK;
        break;
    case MPIDI_RANK_MAP_MLUT_MD_STRIDE_BLOCK:
        if (avtid == 0)
            src->mode = MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA;
        else
            src->mode = MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK;
        break;
    default:
        MPIR_Assert(0);
        break;
    }
    mlut = src->irreg.mlut.t;
    mpi_errno = MPIDIU_alloc_lut(&src->irreg.lut.t, src->size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    src->irreg.lut.lpid = src->irreg.lut.t->lpid;
    for (i = 0; i < src->size; i++) {
        src->irreg.lut.lpid[i] = mlut->gpid[i].lpid;
    }
    MPIDIU_release_mlut(mlut);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " avtid %d", src->avtid));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_CONVERT_MLUT_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_squash_lut_regular_model
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_squash_lut_regular_model(MPIDI_rank_map_t *src,
                                                 MPIDI_rank_map_lut_t **lut)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SQUASH_LUT_REGULAR_MODEL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SQUASH_LUT_REGULAR_MODEL);

    mpi_errno = MPIDIU_alloc_lut(lut, src->size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    switch (src->mode) {
    case MPIDI_RANK_MAP_LUT_STRIDE:
    case MPIDI_RANK_MAP_LUT_STRIDE_INTRA:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_STRIDE_BLOCK_INTRA:
        for (i = 0; i < src->size; i++) {
            (*lut)->lpid[i] =
                src->irreg.lut.lpid[MPIDI_CALC_STRIDE(i,
                                                      src->reg.stride.stride,
                                                      src->reg.stride.blocksize,
                                                      src->reg.stride.offset)];
        }
        break;
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK:
    case MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA:
        for (i = 0; i < src->size; i++) {
            (*lut)->lpid[i] =
                src->irreg.lut.lpid[MPIDI_calc_md_stride(i,
                                                         src->reg.md_stride.dims,
                                                         src->reg.md_stride.params)];
        }
        break;
    default:
        MPIR_Assert(0);
        break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SQUASH_LUT_REGULAR_MODEL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_check_convert_lut_to_regular
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_check_convert_lut_to_regular(MPIDI_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS;
    int mode_detected, offset, blocksize, stride;
    MPIDI_rank_map_lut_t *lut = NULL;
    MPIDI_lpid_t *lut_lpid = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_CONVERT_LUT_TO_REGULAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_CONVERT_LUT_TO_REGULAR);

    if (src->mode < MPIDI_RANK_MAP_LUT
        || src->mode > MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA) {
        goto fn_exit;
    }

    if (src->mode >= MPIDI_RANK_MAP_LUT_STRIDE
        && src->mode <= MPIDI_RANK_MAP_LUT_MD_STRIDE_BLOCK_INTRA) {
        MPIDI_squash_lut_regular_model(src, &lut);
        lut_lpid = lut->lpid;
    }
    else {
        lut_lpid = src->irreg.lut.lpid;
    }
    mode_detected = MPIDI_detect_regular_model(lut_lpid, src->size,
                                               &offset, &blocksize, &stride);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " detected mode: %d", mode_detected));


    switch (mode_detected) {
    case MPIDI_SRC_MAPPER_DIRECT:
        src->mode = MPIDI_RANK_MAP_DIRECT;
        if (src->avtid == 0) {
            src->mode = MPIDI_RANK_MAP_DIRECT_INTRA;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tlut to mode %d", (int) src->mode));
        break;
    case MPIDI_SRC_MAPPER_OFFSET:
        src->mode = MPIDI_RANK_MAP_OFFSET;
        if (src->avtid == 0) {
            src->mode = MPIDI_RANK_MAP_OFFSET_INTRA;
        }
        src->reg.offset = offset;
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
        }
        else {
            src->mode = MPIDI_RANK_MAP_STRIDE_BLOCK;
            if (src->avtid == 0) {
                src->mode = MPIDI_RANK_MAP_STRIDE_BLOCK_INTRA;
            }
        }
        src->reg.stride.stride = stride;
        src->reg.stride.blocksize = blocksize;
        src->reg.stride.offset = offset;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "  lut to mode %d", (int) src->mode));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\toffset: %d", src->reg.stride.offset));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tblocksize: %d", src->reg.stride.blocksize));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tstride: %d", src->reg.stride.stride));
        break;
    case MPIDI_SRC_MAPPER_MD_STRIDE:
        if (src->avtid == 0) {
            src->mode = MPIDI_RANK_MAP_MD_STRIDE_BLOCK_INTRA;
        } else {
            src->mode = MPIDI_RANK_MAP_MD_STRIDE_BLOCK;
        }
        src->reg.md_stride.dims = MPIDI_md_stride_dim;
        mpi_errno = MPIDI_alloc_md_stride_params(&src->reg.md_stride.params_p,
                                                 MPIDI_md_stride_dim);
        src->reg.md_stride.params =
            src->reg.md_stride.params_p->params + src->reg.md_stride.dims * 3;
        memcpy(src->reg.md_stride.params, MPIDI_md_stride_params,
               (3 * src->reg.md_stride.dims) * sizeof(int));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "  lut to mode %d", (int) src->mode));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tdims: %d", src->reg.md_stride.dims));
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tparams: %p", src->reg.md_stride.params));
        break;
    default:
        goto fn_exit;
    }

    MPIDIU_release_lut(src->irreg.lut.t);
    src->irreg.lut.t = NULL;
    src->irreg.lut.lpid = NULL;

  fn_exit:
    if (lut) {
        MPIDIU_release_lut(lut);
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_CONVERT_LUT_TO_REGULAR);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_set_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_set_map(MPIDI_rank_map_t * src_rmap,
                                MPIDI_rank_map_t * dest_rmap,
                                MPIR_Comm_map_t * mapper,
                                int src_comm_size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SET_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SET_MAP);

    /* Simplest case: MAP_DUP, exact duplication of src_comm */
    if (mapper->type == MPIR_COMM_MAP_TYPE__DUP && src_comm_size == total_mapper_size) {
        MPIDI_direct_of_src_rmap(src_rmap, dest_rmap, mapper);
        goto fn_exit;
    }
    /* single src_comm, newcomm is smaller than src_comm, only one mapper */
    else if (mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR &&
             mapper->src_mapping_size == total_mapper_size) {
        /* check if new comm has the same mapping as src_comm */
        /* detect src_mapping_offset for direct_to_direct and offset_to_offset */
        int mode_detected, offset, blocksize, stride;
        mode_detected = MPIDI_detect_regular_model(mapper->src_mapping, mapper->src_mapping_size,
                                                   &offset, &blocksize, &stride);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tdetected mode: %d", mode_detected));

        switch (mode_detected) {
        case MPIDI_SRC_MAPPER_DIRECT:
            MPIDI_direct_of_src_rmap(src_rmap, dest_rmap, mapper);
            break;
        case MPIDI_SRC_MAPPER_OFFSET:
            MPIDI_offset_of_src_rmap(src_rmap, dest_rmap, mapper, offset);
            break;
        case MPIDI_SRC_MAPPER_STRIDE:
            MPIDI_stride_of_src_rmap(src_rmap, dest_rmap, mapper, stride, blocksize, offset);
            break;
        case MPIDI_SRC_MAPPER_MD_STRIDE:
            MPIDI_md_stride_of_src_rmap(src_rmap, dest_rmap, mapper);
            break;
        default:
            if (src_rmap->mode == MPIDI_RANK_MAP_MLUT) {
                MPIDI_src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size,
                                       mapper_offset);
            }
            else {
                MPIDI_src_map_to_lut(src_rmap, dest_rmap, mapper, mapper->src_mapping_size,
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
                         MPIDI_map_size(*mapper), src_comm_size));
        MPIDI_src_comm_to_mlut(src_rmap, dest_rmap, src_comm_size,
                               total_mapper_size, mapper_offset);
    }
    else {      /* mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR */
        MPIDI_src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size, mapper_offset);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SET_MAP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_comm_create_rank_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_comm_create_rank_map(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm_map_t *mapper;
    MPIR_Comm *src_comm;
    int total_mapper_size, mapper_offset;


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_COMM_CREATE_RANK_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_COMM_CREATE_RANK_MAP);

    /* do some sanity checks */
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
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
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2R || mapper->dir == MPIR_COMM_MAP_DIR__R2R)
            continue;

        total_mapper_size += MPIDI_map_size(*mapper);
    }
    mapper_offset = 0;
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
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
                MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                              src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                     comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, local_map), mapper,
                              src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else if (src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM &&
                     comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, map), mapper,
                              src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else {      /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, local_map),
                              mapper, src_comm->local_size, total_mapper_size, mapper_offset);
            }
        }
        else {  /* mapper->dir == MPIR_COMM_MAP_DIR__R2L */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);

            if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->intra, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                              src_comm->remote_size, total_mapper_size, mapper_offset);
            }
            else {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->inter, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, local_map), mapper,
                              src_comm->remote_size, total_mapper_size, mapper_offset);
            }
        }

        mapper_offset += MPIDI_map_size(*mapper);
    }

    /* Next, handle all the mappers that contribute to the remote part
     * of the comm (only valid for intercomms)
     */
    total_mapper_size = 0;
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2L || mapper->dir == MPIR_COMM_MAP_DIR__R2L)
            continue;

        total_mapper_size += MPIDI_map_size(*mapper);
    }
    mapper_offset = 0;
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
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
                MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                              src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else {      /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDI_set_map(&MPIDI_COMM(src_comm, local_map), &MPIDI_COMM(comm, map), mapper,
                              src_comm->local_size, total_mapper_size, mapper_offset);
            }
        }
        else {  /* mapper->dir == MPIR_COMM_MAP_DIR__R2R */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST,
                             " inter->, R2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                             src_comm->remote_size, total_mapper_size, mapper_offset));
            MPIDI_set_map(&MPIDI_COMM(src_comm, map), &MPIDI_COMM(comm, map), mapper,
                          src_comm->remote_size, total_mapper_size, mapper_offset);
        }

        mapper_offset += MPIDI_map_size(*mapper);
    }

    /* check before finishing
     * 1. if mlut can be converted to lut: all avtids are the same
     * 2. if lut can be converted to regular modes: direct, offset, and more
     */
    MPIDI_check_convert_mlut_to_lut(&MPIDI_COMM(comm, map));
    MPIDI_check_convert_lut_to_regular(&MPIDI_COMM(comm, map));
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        MPIDI_check_convert_mlut_to_lut(&MPIDI_COMM(comm, local_map));
        MPIDI_check_convert_lut_to_regular(&MPIDI_COMM(comm, local_map));
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* setup the lut for the local_comm in the intercomm */
        if (comm->local_comm) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\t create local_comm using src_comm"));
            MPIDI_direct_of_src_rmap(&MPIDI_COMM(comm, local_map),
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
    int avtid_, lpid_;
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_COMM_CREATE_RANK_MAP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_check_disjoint_lupids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_check_disjoint_lupids(int lupids1[], int n1,
                                                         int lupids2[], int n2)
{
    int i, mask_size, idx, bit, maxlupid = -1;
    int mpi_errno = MPI_SUCCESS;
    uint32_t lupidmaskPrealloc[128];
    uint32_t *lupidmask;
    MPIR_CHKLMEM_DECL(1);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);

    /* Find the max lupid */
    for (i = 0; i < n1; i++) {
        if (lupids1[i] > maxlupid)
            maxlupid = lupids1[i];
    }
    for (i = 0; i < n2; i++) {
        if (lupids2[i] > maxlupid)
            maxlupid = lupids2[i];
    }

    mask_size = (maxlupid / 32) + 1;

    if (mask_size > 128) {
        MPIR_CHKLMEM_MALLOC(lupidmask, uint32_t *, mask_size * sizeof(uint32_t),
                            mpi_errno, "lupidmask");
    }
    else {
        lupidmask = lupidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(lupidmask, 0x00, mask_size * sizeof(*lupidmask));

    /* Set the bits for the first array */
    for (i = 0; i < n1; i++) {
        idx = lupids1[i] / 32;
        bit = lupids1[i] % 32;
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (i = 0; i < n2; i++) {
        idx = lupids2[i] / 32;
        bit = lupids2[i] % 32;
        if (lupidmask[idx] & (1 << bit)) {
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", lupids2[i]);
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CHECK_DISJOINT_LUPIDS);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4I_COMM_H_INCLUDED */

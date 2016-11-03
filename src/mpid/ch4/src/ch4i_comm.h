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
#define FUNCNAME MPIDII_alloc_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_alloc_lut(MPIDII_rank_map_lut_t ** lut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDII_rank_map_lut_t *new_lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_LUT);

    new_lut = (MPIDII_rank_map_lut_t *) MPL_malloc(sizeof(MPIDII_rank_map_lut_t)
                                                   + size * sizeof(MPIDII_lpid_t));
    if (new_lut == NULL) {
        *lut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_lut, 1);
    *lut = new_lut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc lut %p, size %ld, refcount=%d",
                     new_lut, size * sizeof(MPIDII_lpid_t), MPIR_Object_get_ref(new_lut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_release_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_release_lut(MPIDII_rank_map_lut_t * lut)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_LUT);

    MPIR_Object_release_ref(lut, &count);
    if (count == 0) {
        MPL_free(lut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free lut %p", lut));
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_LUT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_alloc_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_alloc_mlut(MPIDII_rank_map_mlut_t ** mlut, int size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDII_rank_map_mlut_t *new_mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOC_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOC_MLUT);

    new_mlut = (MPIDII_rank_map_mlut_t *) MPL_malloc(sizeof(MPIDII_rank_map_mlut_t)
                                                     + size * sizeof(MPIDII_gpid_t));
    if (new_mlut == NULL) {
        *mlut = NULL;
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    MPIR_Object_set_ref(new_mlut, 1);
    *mlut = new_mlut;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                    (MPL_DBG_FDEST, "alloc mlut %p, size %ld, refcount=%d",
                     new_mlut, size * sizeof(MPIDII_gpid_t), MPIR_Object_get_ref(new_mlut)));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOC_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDIU_release_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDIU_release_mlut(MPIDII_rank_map_mlut_t * mlut)
{
    int mpi_errno = MPI_SUCCESS;
    int count = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_RELEASE_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_RELEASE_MLUT);

    MPIR_Object_release_ref(mlut, &count);
    if (count == 0) {
        MPL_free(mlut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE, (MPL_DBG_FDEST, "free mlut %p", mlut));
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_RELEASE_MLUT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_map_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_map_size(MPIR_Comm_map_t map)
{
    if (map.type == MPIR_COMM_MAP_TYPE__IRREGULAR)
        return map.src_mapping_size;
    else if (map.dir == MPIR_COMM_MAP_DIR__L2L || map.dir == MPIR_COMM_MAP_DIR__L2R)
        return map.src_comm->local_size;
    else
        return map.src_comm->remote_size;
}

/*
 * This enum is used exclusively in this header file
 */
enum MPIDII_src_mapper_models {
    MPIDII_SRC_MAPPER_IRREGULAR = 0,
    MPIDII_SRC_MAPPER_DIRECT = 1,
    MPIDII_SRC_MAPPER_OFFSET = 2,
    MPIDII_SRC_MAPPER_STRIDE = 3
};

#undef FUNCNAME
#define FUNCNAME MPIDII_detect_regular_model
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_detect_regular_model(int *lpid, int size,
                                              int *offset, int *blocksize, int *stride)
{
    int off = 0, bs = 0, st = 0;
    int i;

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
            return MPIDII_SRC_MAPPER_DIRECT;
        }
        else {
            *offset = off;
            return MPIDII_SRC_MAPPER_OFFSET;
        }
    }

    /* blocksize less than total size, try if this is stride */
    st = lpid[bs] - lpid[0];
    if (st < 0 || st <= bs) {
        return MPIDII_SRC_MAPPER_IRREGULAR;
    }
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tdetect model: stride %d", st));
    for (i = bs; i < size; i++) {
        if (lpid[i] != MPIDII_CALC_STRIDE(i, st, bs, off)) {
            return MPIDII_SRC_MAPPER_IRREGULAR;
        }
    }
    *offset = off;
    *blocksize = bs;
    *stride = st;
    return MPIDII_SRC_MAPPER_STRIDE;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_src_comm_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_src_comm_to_lut(MPIDII_rank_map_t * src,
                                         MPIDII_rank_map_t * dest,
                                         int size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIDII_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_SRC_COMM_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_SRC_COMM_TO_LUT);

    if (!mapper_offset) {
        mpi_errno = MPIDII_alloc_lut(&lut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
        dest->mode = MPIDII_RANK_MAP_LUT;
        dest->avtid = src->avtid;
        dest->irreg.lut.t = lut;
        dest->irreg.lut.lpid = lut->lpid;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " size %d", size));
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT:
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = i;
        }
        break;
    case MPIDII_RANK_MAP_OFFSET:
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = i + src->reg.offset;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
        break;
    case MPIDII_RANK_MAP_STRIDE:
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = MPIDII_CALC_STRIDE_SIMPLE(i,
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
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = MPIDII_CALC_STRIDE(i,
                                                                         src->reg.stride.stride,
                                                                         src->reg.stride.blocksize,
                                                                         src->reg.stride.offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[i];
        }
        break;
    case MPIDII_RANK_MAP_MLUT:
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_SRC_COMM_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_src_comm_to_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_src_comm_to_mlut(MPIDII_rank_map_t * src,
                                          MPIDII_rank_map_t * dest,
                                          int size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    MPIDII_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_SRC_COMM_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_SRC_COMM_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIDII_alloc_mlut(&mlut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
        dest->mode = MPIDII_RANK_MAP_MLUT;
        dest->avtid = -1;
        dest->irreg.mlut.t = mlut;
        dest->irreg.mlut.gpid = mlut->gpid;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " size %d", size));
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT:
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = i;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDII_RANK_MAP_OFFSET:
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = i + src->reg.offset;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
        break;
    case MPIDII_RANK_MAP_STRIDE:
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = MPIDII_CALC_STRIDE_SIMPLE(i,
                                                                                      src->reg.
                                                                                      stride.stride,
                                                                                      src->reg.
                                                                                      stride.
                                                                                      offset);
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = MPIDII_CALC_STRIDE(i,
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
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.lut.lpid[i];
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->avtid;
        }
        break;
    case MPIDII_RANK_MAP_MLUT:
        for (i = 0; i < size; i++) {
            dest->irreg.mlut.gpid[i + mapper_offset].lpid = src->irreg.mlut.gpid[i].lpid;
            dest->irreg.mlut.gpid[i + mapper_offset].avtid = src->irreg.mlut.gpid[i].avtid;
        }
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_SRC_COMM_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_src_mlut_to_mlut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_src_mlut_to_mlut(MPIDII_rank_map_t * src,
                                          MPIDII_rank_map_t * dest,
                                          MPIR_Comm_map_t * mapper,
                                          int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = MPIDII_map_size(*mapper);
    MPIDII_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_MLUT_TO_MLUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_MLUT_TO_MLUT);

    if (!mapper_offset) {
        mpi_errno = MPIDII_alloc_mlut(&mlut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
    }

    dest->mode = MPIDII_RANK_MAP_MLUT;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_MLUT_TO_MLUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_src_map_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_src_map_to_lut(MPIDII_rank_map_t * src,
                                        MPIDII_rank_map_t * dest,
                                        MPIR_Comm_map_t * mapper,
                                        int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS, i;
    int size = MPIDII_map_size(*mapper);
    MPIDII_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_MAP_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_MAP_TO_LUT);

    if (!mapper_offset) {
        mpi_errno = MPIDII_alloc_lut(&lut, total_mapper_size);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        dest->size = total_mapper_size;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " size %d, mapper->src_mapping_size %d",
                     size, mapper->src_mapping_size));
    dest->mode = MPIDII_RANK_MAP_LUT;
    dest->avtid = src->avtid;
    dest->irreg.lut.t = lut;
    dest->irreg.lut.lpid = lut->lpid;
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT:
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i];
        }
        break;
    case MPIDII_RANK_MAP_OFFSET:
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = mapper->src_mapping[i] + src->reg.offset;
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source offset %d", src->reg.offset));
        break;
    case MPIDII_RANK_MAP_STRIDE:
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] =
                MPIDII_CALC_STRIDE_SIMPLE(mapper->src_mapping[i], src->reg.stride.stride,
                                          src->reg.stride.offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = MPIDII_CALC_STRIDE(mapper->src_mapping[i],
                                                                         src->reg.stride.stride,
                                                                         src->reg.stride.blocksize,
                                                                         src->reg.stride.offset);
        }
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " source stride %d blocksize %d offset %d",
                         src->reg.stride.stride, src->reg.stride.blocksize,
                         src->reg.stride.offset));
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        for (i = 0; i < size; i++) {
            dest->irreg.lut.lpid[i + mapper_offset] = src->irreg.lut.lpid[mapper->src_mapping[i]];
        }
        break;
    default:
        mpi_errno = 1;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, " cannot convert mode %d to lut", (int) src->mode));
        goto fn_fail;
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_MAP_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_direct_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDII_direct_of_src_rmap(MPIDII_rank_map_t * src,
                                             MPIDII_rank_map_t * dest, MPIR_Comm_map_t * mapper)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_DIRECT_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_DIRECT_OF_SRC_RMAP);
    dest->mode = src->mode;
    dest->size = MPIDII_map_size(*mapper);
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT:
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        break;
    case MPIDII_RANK_MAP_OFFSET:
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        dest->reg.offset = src->reg.offset;
        break;
    case MPIDII_RANK_MAP_STRIDE:
    case MPIDII_RANK_MAP_STRIDE_INTRA:
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset;
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = src->irreg.lut.lpid;
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDII_RANK_MAP_MLUT:
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = src->irreg.mlut.gpid;
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_DIRECT_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDII_offset_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDII_offset_of_src_rmap(MPIDII_rank_map_t * src,
                                             MPIDII_rank_map_t * dest,
                                             MPIR_Comm_map_t * mapper, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_OFFSET_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_OFFSET_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    dest->size = MPIDII_map_size(*mapper);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        dest->mode = MPIDII_RANK_MAP_OFFSET_INTRA;
        dest->reg.offset = offset;
        break;
    case MPIDII_RANK_MAP_DIRECT:
        dest->mode = MPIDII_RANK_MAP_OFFSET;
        dest->reg.offset = offset;
        break;
    case MPIDII_RANK_MAP_OFFSET:
        dest->mode = MPIDII_RANK_MAP_OFFSET;
        dest->reg.offset = src->reg.offset + offset;
        break;
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        dest->mode = MPIDII_RANK_MAP_OFFSET_INTRA;
        dest->reg.offset = src->reg.offset + offset;
        break;
    case MPIDII_RANK_MAP_STRIDE:
        dest->mode = MPIDII_RANK_MAP_STRIDE;
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
        break;
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        dest->mode = MPIDII_RANK_MAP_STRIDE_INTRA;
        dest->reg.stride.stride = src->reg.stride.stride;
        dest->reg.stride.blocksize = src->reg.stride.blocksize;
        dest->reg.stride.offset = src->reg.stride.offset + offset * src->reg.stride.stride;
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        MPIDII_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        dest->mode = src->mode;
        dest->irreg.lut.t = src->irreg.lut.t;
        dest->irreg.lut.lpid = &src->irreg.lut.lpid[offset];
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tref count %d", MPIR_Object_get_ref(src->irreg.lut.t)));
        MPIR_Object_add_ref(src->irreg.lut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src lut"));
        break;
    case MPIDII_RANK_MAP_MLUT:
        dest->mode = src->mode;
        dest->irreg.mlut.t = src->irreg.mlut.t;
        dest->irreg.mlut.gpid = &src->irreg.mlut.gpid[offset];
        MPIR_Object_add_ref(src->irreg.mlut.t);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, "\tadd ref to src mlut"));
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_OFFSET_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDII_stride_of_src_rmap
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline void MPIDII_stride_of_src_rmap(MPIDII_rank_map_t * src,
                                             MPIDII_rank_map_t * dest,
                                             MPIR_Comm_map_t * mapper,
                                             int stride, int blocksize, int offset)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_STRIDE_OF_SRC_RMAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_STRIDE_OF_SRC_RMAP);
    dest->avtid = src->avtid;
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " source mode %d", (int) src->mode));
    switch (src->mode) {
    case MPIDII_RANK_MAP_DIRECT_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE_INTRA;
        }
        else {
            dest->mode = MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA;
        }
        dest->size = MPIDII_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset;
        MPIR_Assert(stride > 0);
        MPIR_Assert(blocksize > 0);
        break;
    case MPIDII_RANK_MAP_DIRECT:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE;
        }
        else {
            dest->mode = MPIDII_RANK_MAP_STRIDE_BLOCK;
        }
        dest->size = MPIDII_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset;
        MPIR_Assert(stride > 0);
        MPIR_Assert(blocksize > 0);
        break;
    case MPIDII_RANK_MAP_OFFSET:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE;
        }
        else {
            dest->mode = MPIDII_RANK_MAP_STRIDE_BLOCK;
        }
        dest->size = MPIDII_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset + src->reg.offset;
        break;
    case MPIDII_RANK_MAP_OFFSET_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE_INTRA;
        }
        else {
            dest->mode = MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA;
        }
        dest->size = MPIDII_map_size(*mapper);
        dest->reg.stride.stride = stride;
        dest->reg.stride.blocksize = blocksize;
        dest->reg.stride.offset = offset + src->reg.offset;
        break;
    case MPIDII_RANK_MAP_STRIDE:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE;
            dest->reg.stride.stride = src->reg.stride.stride * stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
        }
        else {
            MPIDII_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        }
        break;
    case MPIDII_RANK_MAP_STRIDE_INTRA:
        if (blocksize == 1) {
            dest->mode = MPIDII_RANK_MAP_STRIDE_INTRA;
            dest->reg.stride.stride = src->reg.stride.stride * stride;
            dest->reg.stride.blocksize = blocksize;
            dest->reg.stride.offset = src->reg.stride.stride * offset + src->reg.stride.offset;
        }
        else {
            MPIDII_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        }
        break;
    case MPIDII_RANK_MAP_STRIDE_BLOCK:
    case MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA:
        MPIDII_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDII_RANK_MAP_LUT:
    case MPIDII_RANK_MAP_LUT_INTRA:
        MPIDII_src_map_to_lut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDII_RANK_MAP_MLUT:
        MPIDII_src_mlut_to_mlut(src, dest, mapper, mapper->src_mapping_size, 0);
        break;
    case MPIDII_RANK_MAP_NONE:
        MPIR_Assert(0);
        break;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_STRIDE_OF_SRC_RMAP);
}

#undef FUNCNAME
#define FUNCNAME MPIDII_check_convert_mlut_to_lut
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_check_convert_mlut_to_lut(MPIDII_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS, i;
    int flag = 1;
    int avtid;
    MPIDII_rank_map_mlut_t *mlut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_CONVERT_MLUT_TO_LUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_CONVERT_MLUT_TO_LUT);

    if (src->mode != MPIDII_RANK_MAP_MLUT) {
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
        src->mode = MPIDII_RANK_MAP_LUT_INTRA;
    }
    else {
        src->mode = MPIDII_RANK_MAP_LUT;
    }
    mlut = src->irreg.mlut.t;
    mpi_errno = MPIDII_alloc_lut(&src->irreg.lut.t, src->size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    src->irreg.lut.lpid = src->irreg.lut.t->lpid;
    for (i = 0; i < src->size; i++) {
        src->irreg.lut.lpid[i] = mlut->gpid[i].lpid;
    }
    MPIDIU_release_mlut(mlut);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE, (MPL_DBG_FDEST, " avtid %d", src->avtid));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_CONVERT_MLUT_TO_LUT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_check_convert_lut_to_regular
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_check_convert_lut_to_regular(MPIDII_rank_map_t * src)
{
    int mpi_errno = MPI_SUCCESS;
    int mode_detected, offset, blocksize, stride;
    MPIDII_rank_map_lut_t *lut = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_CONVERT_LUT_TO_REGULAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_CONVERT_LUT_TO_REGULAR);

    if (src->mode != MPIDII_RANK_MAP_LUT && src->mode != MPIDII_RANK_MAP_LUT_INTRA) {
        goto fn_exit;
    }

    lut = src->irreg.lut.t;
    mode_detected = MPIDII_detect_regular_model(src->irreg.lut.lpid, src->size,
                                                &offset, &blocksize, &stride);
    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                    (MPL_DBG_FDEST, " detected mode: %d", mode_detected));


    switch (mode_detected) {
    case MPIDII_SRC_MAPPER_DIRECT:
        src->mode = MPIDII_RANK_MAP_DIRECT;
        if (src->avtid == 0) {
            src->mode = MPIDII_RANK_MAP_DIRECT_INTRA;
        }
        src->irreg.lut.t = NULL;
        src->irreg.lut.lpid = NULL;
        MPIDIU_release_lut(lut);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tlut to mode %d", (int) src->mode));
        break;
    case MPIDII_SRC_MAPPER_OFFSET:
        src->mode = MPIDII_RANK_MAP_OFFSET;
        if (src->avtid == 0) {
            src->mode = MPIDII_RANK_MAP_OFFSET_INTRA;
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
    case MPIDII_SRC_MAPPER_STRIDE:
        if (blocksize == 1) {
            src->mode = MPIDII_RANK_MAP_STRIDE;
            if (src->avtid == 0) {
                src->mode = MPIDII_RANK_MAP_STRIDE_INTRA;
            }
        }
        else {
            src->mode = MPIDII_RANK_MAP_STRIDE_BLOCK;
            if (src->avtid == 0) {
                src->mode = MPIDII_RANK_MAP_STRIDE_BLOCK_INTRA;
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_CONVERT_LUT_TO_REGULAR);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_set_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDII_set_map(MPIDII_rank_map_t * src_rmap,
                                 MPIDII_rank_map_t * dest_rmap,
                                 MPIR_Comm_map_t * mapper,
                                 int src_comm_size, int total_mapper_size, int mapper_offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_SET_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_SET_MAP);

    /* Simplest case: MAP_DUP, exact duplication of src_comm */
    if (mapper->type == MPIR_COMM_MAP_TYPE__DUP && src_comm_size == total_mapper_size) {
        MPIDII_direct_of_src_rmap(src_rmap, dest_rmap, mapper);
        goto fn_exit;
    }
    /* single src_comm, newcomm is smaller than src_comm, only one mapper */
    else if (mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR &&
             mapper->src_mapping_size == total_mapper_size) {
        /* check if new comm has the same mapping as src_comm */
        /* detect src_mapping_offset for direct_to_direct and offset_to_offset */
        int mode_detected, offset, blocksize, stride;
        mode_detected = MPIDII_detect_regular_model(mapper->src_mapping, mapper->src_mapping_size,
                                                    &offset, &blocksize, &stride);
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                        (MPL_DBG_FDEST, "\tdetected mode: %d", mode_detected));

        switch (mode_detected) {
        case MPIDII_SRC_MAPPER_DIRECT:
            MPIDII_direct_of_src_rmap(src_rmap, dest_rmap, mapper);
            break;
        case MPIDII_SRC_MAPPER_OFFSET:
            MPIDII_offset_of_src_rmap(src_rmap, dest_rmap, mapper, offset);
            break;
        case MPIDII_SRC_MAPPER_STRIDE:
            MPIDII_stride_of_src_rmap(src_rmap, dest_rmap, mapper, stride, blocksize, offset);
            break;
        default:
            if (src_rmap->mode == MPIDII_RANK_MAP_MLUT) {
                MPIDII_src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size,
                                        mapper_offset);
            }
            else {
                MPIDII_src_map_to_lut(src_rmap, dest_rmap, mapper, mapper->src_mapping_size,
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
                         MPIDII_map_size(*mapper), src_comm_size));
        MPIDII_src_comm_to_mlut(src_rmap, dest_rmap, src_comm_size,
                                total_mapper_size, mapper_offset);
    }
    else {      /* mapper->type == MPIR_COMM_MAP_TYPE__IRREGULAR */
        MPIDII_src_mlut_to_mlut(src_rmap, dest_rmap, mapper, total_mapper_size, mapper_offset);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_SET_MAP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_comm_create_rank_map
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDII_comm_create_rank_map(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm_map_t *mapper;
    MPIR_Comm *src_comm;
    int total_mapper_size, mapper_offset;


    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDII_COMM_CREATE_RANK_MAP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDII_COMM_CREATE_RANK_MAP);

    /* do some sanity checks */
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            MPIR_Assert(mapper->dir == MPIR_COMM_MAP_DIR__L2L ||
                        mapper->dir == MPIR_COMM_MAP_DIR__L2R);

        if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            MPIR_Assert(mapper->dir == MPIR_COMM_MAP_DIR__L2L ||
                        mapper->dir == MPIR_COMM_MAP_DIR__R2L);
    }

    /* First, handle all the mappers that contribute to the local part
     * of the comm */
    total_mapper_size = 0;
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2R || mapper->dir == MPIR_COMM_MAP_DIR__R2R)
            continue;

        total_mapper_size += MPIDII_map_size(*mapper);
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
                MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, map), mapper,
                               src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else if (src_comm->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                     comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " intra->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, local_map), mapper,
                               src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else if (src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM &&
                     comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->intra, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDII_set_map(&MPIDII_COMM(src_comm, local_map), &MPIDII_COMM(comm, map), mapper,
                               src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else {      /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM && comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->inter, L2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDII_set_map(&MPIDII_COMM(src_comm, local_map), &MPIDII_COMM(comm, local_map),
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
                MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, map), mapper,
                               src_comm->remote_size, total_mapper_size, mapper_offset);
            }
            else {
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " ->inter, R2L, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->remote_size, total_mapper_size, mapper_offset));
                MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, local_map), mapper,
                               src_comm->remote_size, total_mapper_size, mapper_offset);
            }
        }

        mapper_offset += MPIDII_map_size(*mapper);
    }

    /* Next, handle all the mappers that contribute to the remote part
     * of the comm (only valid for intercomms)
     */
    total_mapper_size = 0;
    MPL_LL_FOREACH(comm->mapper_head, mapper) {
        if (mapper->dir == MPIR_COMM_MAP_DIR__L2L || mapper->dir == MPIR_COMM_MAP_DIR__R2L)
            continue;

        total_mapper_size += MPIDII_map_size(*mapper);
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
                MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, map), mapper,
                               src_comm->local_size, total_mapper_size, mapper_offset);
            }
            else {      /* src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM */
                MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                                (MPL_DBG_FDEST,
                                 " inter->, L2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                                 src_comm->local_size, total_mapper_size, mapper_offset));
                MPIDII_set_map(&MPIDII_COMM(src_comm, local_map), &MPIDII_COMM(comm, map), mapper,
                               src_comm->local_size, total_mapper_size, mapper_offset);
            }
        }
        else {  /* mapper->dir == MPIR_COMM_MAP_DIR__R2R */
            MPIR_Assert(src_comm->comm_kind == MPIR_COMM_KIND__INTERCOMM);
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST,
                             " inter->, R2R, size=%d, total_mapper_size=%d, mapper_offset=%d",
                             src_comm->remote_size, total_mapper_size, mapper_offset));
            MPIDII_set_map(&MPIDII_COMM(src_comm, map), &MPIDII_COMM(comm, map), mapper,
                           src_comm->remote_size, total_mapper_size, mapper_offset);
        }

        mapper_offset += MPIDII_map_size(*mapper);
    }

    /* check before finishing
     * 1. if mlut can be converted to lut: all avtids are the same
     * 2. if lut can be converted to regular modes: direct, offset, and more
     */
    MPIDII_check_convert_mlut_to_lut(&MPIDII_COMM(comm, map));
    MPIDII_check_convert_lut_to_regular(&MPIDII_COMM(comm, map));
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        MPIDII_check_convert_mlut_to_lut(&MPIDII_COMM(comm, local_map));
        MPIDII_check_convert_lut_to_regular(&MPIDII_COMM(comm, local_map));
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        /* setup the lut for the local_comm in the intercomm */
        if (comm->local_comm) {
            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MAP, VERBOSE,
                            (MPL_DBG_FDEST, "\t create local_comm using src_comm"));
            MPIDII_direct_of_src_rmap(&MPIDII_COMM(comm, local_map),
                                      &MPIDII_COMM(comm->local_comm, map), mapper);

            MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_MEMORY, VERBOSE,
                            (MPL_DBG_FDEST, "create local_comm using src_comm"));
        }
    }

    if (comm->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        MPIDII_COMM(comm, local_map).mode = MPIDII_RANK_MAP_NONE;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDII_COMM_CREATE_RANK_MAP);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDII_check_disjoint_lupids
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDII_check_disjoint_lupids(int lupids1[], int n1,
                                                          int lupids2[], int n2)
{
    int i, mask_size, idx, bit, maxlupid = -1;
    int mpi_errno = MPI_SUCCESS;
    uint32_t lupidmaskPrealloc[128];
    uint32_t *lupidmask;
    MPIR_CHKLMEM_DECL(1);

    /* Find the max lupid */
    for (i=0; i<n1; i++) {
        if (lupids1[i] > maxlupid) maxlupid = lupids1[i];
    }
    for (i=0; i<n2; i++) {
        if (lupids2[i] > maxlupid) maxlupid = lupids2[i];
    }

    mask_size = (maxlupid / 32) + 1;

    if (mask_size > 128) {
        MPIR_CHKLMEM_MALLOC(lupidmask,uint32_t*,mask_size*sizeof(uint32_t),
                            mpi_errno,"lupidmask");
    }
    else {
        lupidmask = lupidmaskPrealloc;
    }

    /* zero the bitvector array */
    memset(lupidmask, 0x00, mask_size*sizeof(*lupidmask));

    /* Set the bits for the first array */
    for (i=0; i<n1; i++) {
        idx = lupids1[i] / 32;
        bit = lupids1[i] % 32;
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Look for any duplicates in the second array */
    for (i=0; i<n2; i++) {
        idx = lupids2[i] / 32;
        bit = lupids2[i] % 32;
        if (lupidmask[idx] & (1 << bit)) {
            MPIR_ERR_SET1(mpi_errno,MPI_ERR_COMM,
                          "**dupprocesses", "**dupprocesses %d", lupids2[i] );
            goto fn_fail;
        }
        /* Add a check on duplicates *within* group 2 */
        lupidmask[idx] = lupidmask[idx] | (1 << bit);
        MPIR_Assert(idx < mask_size);
    }

    /* Also fall through for normal return */
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* ifndef CH4I_COMM_H_INCLUDED */

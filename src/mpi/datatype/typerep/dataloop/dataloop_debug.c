/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "dataloop_internal.h"
#include <stdlib.h>
#include <limits.h>
#include "datatype.h"

#define NR_TYPE_CUTOFF 6        /* Number of types to display before truncating
                                 * output. 6 picked as arbitrary cutoff */

/* note: this isn't really "error handling" per se, but leave these comments
 * because Bill uses them for coverage analysis.
 */

/* --BEGIN ERROR HANDLING-- */
static void dot_printf(MPII_Dataloop * loop_p, int depth, int header);
static void dot_printf(MPII_Dataloop * loop_p, int depth, int header)
{
    int i;

    if (loop_p == NULL) {
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "<null dataloop>\n"));
        return;
    }

    if (header) {
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                            /* graphviz does not like the 0xNNN format */
                                            "digraph %lld {   {", (long long int) loop_p));
    }

    switch (loop_p->kind & MPII_DATALOOP_KIND_MASK) {
        case MPII_DATALOOP_KIND_CONTIG:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "      dl%d [shape = record, label = \"contig |{ ct = %d; el_sz = "
                                                MPI_AINT_FMT_DEC_SPEC "; el_ext = "
                                                MPI_AINT_FMT_DEC_SPEC " }\"];", depth,
                                                (int) loop_p->loop_params.c_t.count,
                                                (MPI_Aint) loop_p->el_size,
                                                (MPI_Aint) loop_p->el_extent));
            break;
        case MPII_DATALOOP_KIND_VECTOR:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "      dl%d [shape = record, label = \"vector |{ ct = %d; blk = %d; str = "
                                                MPI_AINT_FMT_DEC_SPEC "; el_sz = "
                                                MPI_AINT_FMT_DEC_SPEC "; el_ext =  "
                                                MPI_AINT_FMT_DEC_SPEC " }\"];", depth,
                                                (int) loop_p->loop_params.v_t.count,
                                                (int) loop_p->loop_params.v_t.blocksize,
                                                (MPI_Aint) loop_p->loop_params.v_t.stride,
                                                (MPI_Aint) loop_p->el_size,
                                                (MPI_Aint) loop_p->el_extent));
            break;
        case MPII_DATALOOP_KIND_INDEXED:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "      dl%d [shape = record, label = \"indexed |{ ct = %d; tot_blks = %d; regions = ",
                                                depth,
                                                (int) loop_p->loop_params.i_t.count,
                                                (int) loop_p->loop_params.i_t.total_blocks));

            for (i = 0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.i_t.count; i++) {
                if (i + 1 < loop_p->loop_params.i_t.count) {
                    /* more regions after this one */
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                        "\\n(" MPI_AINT_FMT_DEC_SPEC ", %d), ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        i_t.offset_array[i],
                                                        (int) loop_p->loop_params.
                                                        i_t.blocksize_array[i]));
                } else {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                        "\\n(" MPI_AINT_FMT_DEC_SPEC ", %d); ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        i_t.offset_array[i],
                                                        (int) loop_p->loop_params.
                                                        i_t.blocksize_array[i]));
                }
            }
            if (i < loop_p->loop_params.i_t.count) {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "\\n...; ");
            }

            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "\\nel_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext = "
                                                MPI_AINT_FMT_DEC_SPEC " }\"];\n",
                                                (MPI_Aint) loop_p->el_size,
                                                (MPI_Aint) loop_p->el_extent));
            break;
        case MPII_DATALOOP_KIND_BLOCKINDEXED:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "      dl%d [shape = record, label = \"blockindexed |{ ct = %d; blk = %d; disps = ",
                                                depth,
                                                (int) loop_p->loop_params.bi_t.count,
                                                (int) loop_p->loop_params.bi_t.blocksize));

            for (i = 0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.bi_t.count; i++) {
                if (i + 1 < loop_p->loop_params.bi_t.count) {
                    /* more regions after this one */
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                        MPI_AINT_FMT_DEC_SPEC ",\\n ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        bi_t.offset_array[i]));
                } else {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                        MPI_AINT_FMT_DEC_SPEC "; ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        bi_t.offset_array[i]));
                }
            }
            if (i < loop_p->loop_params.bi_t.count) {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "...; ");
            }

            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "\\nel_sz = " MPI_AINT_FMT_DEC_SPEC "; el_ext = "
                                                MPI_AINT_FMT_DEC_SPEC " }\"];",
                                                (MPI_Aint) loop_p->el_size,
                                                (MPI_Aint) loop_p->el_extent));
            break;
        case MPII_DATALOOP_KIND_STRUCT:
            MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                                "      dl%d [shape = record, label = \"struct | {ct = %d; blks = ",
                                                depth, (int) loop_p->loop_params.s_t.count));
            for (i = 0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.s_t.count; i++) {
                if (i + 1 < loop_p->loop_params.s_t.count) {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "%d, ",
                                                        (int) loop_p->loop_params.
                                                        s_t.blocksize_array[i]));
                } else {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, "%d; ",
                                                        (int) loop_p->loop_params.
                                                        s_t.blocksize_array[i]));
                }
            }
            if (i < loop_p->loop_params.s_t.count) {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "...; disps = ");
            } else {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "disps = ");
            }

            for (i = 0; i < NR_TYPE_CUTOFF && i < loop_p->loop_params.s_t.count; i++) {
                if (i + 1 < loop_p->loop_params.s_t.count) {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, MPI_AINT_FMT_DEC_SPEC ", ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        s_t.offset_array[i]));
                } else {
                    MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST, MPI_AINT_FMT_DEC_SPEC "; ",
                                                        (MPI_Aint) loop_p->loop_params.
                                                        s_t.offset_array[i]));
                }
            }
            if (i < loop_p->loop_params.s_t.count) {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "... }\"];");
            } else {
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "}\"];");
            }
            break;
        default:
            MPIR_Assert(0);
    }

    if (!(loop_p->kind & MPII_DATALOOP_FINAL_MASK)) {
        /* more loops to go; recurse */
        MPL_DBG_OUT_FMT(MPIR_DBG_DATATYPE, (MPL_DBG_FDEST,
                                            "      dl%d -> dl%d;\n", depth, depth + 1));
        switch (loop_p->kind & MPII_DATALOOP_KIND_MASK) {
            case MPII_DATALOOP_KIND_CONTIG:
                dot_printf(loop_p->loop_params.c_t.dataloop, depth + 1, 0);
                break;
            case MPII_DATALOOP_KIND_VECTOR:
                dot_printf(loop_p->loop_params.v_t.dataloop, depth + 1, 0);
                break;
            case MPII_DATALOOP_KIND_INDEXED:
                dot_printf(loop_p->loop_params.i_t.dataloop, depth + 1, 0);
                break;
            case MPII_DATALOOP_KIND_BLOCKINDEXED:
                dot_printf(loop_p->loop_params.bi_t.dataloop, depth + 1, 0);
                break;
            case MPII_DATALOOP_KIND_STRUCT:
                for (i = 0; i < loop_p->loop_params.s_t.count; i++) {
                    dot_printf(loop_p->loop_params.s_t.dataloop_array[i], depth + 1, 0);
                }
                break;
            default:
                MPL_DBG_OUT(MPIR_DBG_DATATYPE, "      < unsupported type >");
        }
    }


    if (header) {
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "   }\n}");
    }
    return;
}

void MPIR_Dataloop_printf(MPI_Datatype type, int depth, int header)
{
    if (HANDLE_IS_BUILTIN(type)) {
        MPL_DBG_OUT(MPIR_DBG_DATATYPE, "type_dot_printf: type is a basic");
        return;
    } else {
        MPIR_Datatype *dt_p;
        MPII_Dataloop *loop_p;

        MPIR_Datatype_get_ptr(type, dt_p);
        loop_p = (MPII_Dataloop *) dt_p->typerep.handle;

        dot_printf(loop_p, depth, header);
        return;
    }
}

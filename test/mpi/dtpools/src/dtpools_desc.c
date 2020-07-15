/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"

static const char *kind_to_name(DTPI_Datatype_kind_e kind)
{
    const char *ret;

    switch (kind) {
        case DTPI_DATATYPE_KIND__CONTIG:
            ret = "contig";
            break;
        case DTPI_DATATYPE_KIND__DUP:
            ret = "dup";
            break;
        case DTPI_DATATYPE_KIND__RESIZED:
            ret = "resized";
            break;
        case DTPI_DATATYPE_KIND__VECTOR:
            ret = "vector";
            break;
        case DTPI_DATATYPE_KIND__HVECTOR:
            ret = "hvector";
            break;
        case DTPI_DATATYPE_KIND__BLKINDX:
            ret = "blkindx";
            break;
        case DTPI_DATATYPE_KIND__BLKHINDX:
            ret = "blkhindx";
            break;
        case DTPI_DATATYPE_KIND__INDEXED:
            ret = "indexed";
            break;
        case DTPI_DATATYPE_KIND__HINDEXED:
            ret = "hindexed";
            break;
        case DTPI_DATATYPE_KIND__SUBARRAY:
            ret = "subarray";
            break;
        case DTPI_DATATYPE_KIND__STRUCT:
            ret = "structure";
            break;
        default:
            ret = NULL;
    }

    return ret;
}

int DTPI_populate_dtp_desc(DTPI_obj_s * obj_priv, DTPI_pool_s * dtp, char **desc_)
{
    char *desc;
    int desclen = 0, maxlen = 16384;
    int depth = 0;
    int tree_depth = 0;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    for (DTPI_Attr_s * attr = obj_priv->attr_tree; attr; attr = attr->child)
        tree_depth++;

    DTPI_ALLOC_OR_FAIL(desc, maxlen, rc);
    DTPI_snprintf(rc, desc, desclen, maxlen, "nesting levels: %d\n", tree_depth);

    for (DTPI_Attr_s * attr = obj_priv->attr_tree; attr; attr = attr->child, depth++) {
        DTPI_snprintf(rc, desc, desclen, maxlen, "%d: %s [", depth, kind_to_name(attr->kind));

        switch (attr->kind) {
            case DTPI_DATATYPE_KIND__CONTIG:
                DTPI_snprintf(rc, desc, desclen, maxlen, "blklen %d", attr->u.contig.blklen);
                break;

            case DTPI_DATATYPE_KIND__DUP:
                break;

            case DTPI_DATATYPE_KIND__RESIZED:
                DTPI_snprintf(rc, desc, desclen, maxlen, "lb %zd, extent %zd",
                              attr->u.resized.lb, attr->u.resized.extent);
                break;

            case DTPI_DATATYPE_KIND__VECTOR:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklen %d, stride %d",
                              attr->u.vector.numblks, attr->u.vector.blklen, attr->u.vector.stride);
                break;

            case DTPI_DATATYPE_KIND__HVECTOR:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklen %d, stride %zd",
                              attr->u.hvector.numblks,
                              attr->u.hvector.blklen, attr->u.hvector.stride);
                break;

            case DTPI_DATATYPE_KIND__BLKINDX:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklen %d, displs (",
                              attr->u.blkindx.numblks, attr->u.blkindx.blklen);
                for (int j = 0; j < attr->u.blkindx.numblks; j++) {
                    if (j >= 10 && j < attr->u.blkindx.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.blkindx.array_of_displs[j]);
                    if (j < attr->u.blkindx.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, ")");
                break;

            case DTPI_DATATYPE_KIND__BLKHINDX:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklen %d, displs (",
                              attr->u.blkhindx.numblks, attr->u.blkhindx.blklen);
                for (int j = 0; j < attr->u.blkhindx.numblks; j++) {
                    if (j >= 10 && j < attr->u.blkhindx.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%zd",
                                  attr->u.blkhindx.array_of_displs[j]);
                    if (j < attr->u.blkhindx.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, ")");
                break;

            case DTPI_DATATYPE_KIND__INDEXED:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklens (",
                              attr->u.indexed.numblks);
                for (int j = 0; j < attr->u.indexed.numblks; j++) {
                    if (j >= 10 && j < attr->u.indexed.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.indexed.array_of_blklens[j]);
                    if (j < attr->u.indexed.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), displs (");
                for (int j = 0; j < attr->u.indexed.numblks; j++) {
                    if (j >= 10 && j < attr->u.indexed.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.indexed.array_of_displs[j]);
                    if (j < attr->u.indexed.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, ")");
                break;

            case DTPI_DATATYPE_KIND__HINDEXED:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklens (",
                              attr->u.hindexed.numblks);
                for (int j = 0; j < attr->u.hindexed.numblks; j++) {
                    if (j >= 10 && j < attr->u.hindexed.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.hindexed.array_of_blklens[j]);
                    if (j < attr->u.hindexed.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), displs (");
                for (int j = 0; j < attr->u.hindexed.numblks; j++) {
                    if (j >= 10 && j < attr->u.hindexed.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%zd",
                                  attr->u.hindexed.array_of_displs[j]);
                    if (j < attr->u.hindexed.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, ")");
                break;

            case DTPI_DATATYPE_KIND__SUBARRAY:
                DTPI_snprintf(rc, desc, desclen, maxlen, "ndims %d, sizes (",
                              attr->u.subarray.ndims);
                for (int j = 0; j < attr->u.subarray.ndims; j++) {
                    if (j >= 10 && j < attr->u.subarray.ndims - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.subarray.array_of_sizes[j]);
                    if (j < attr->u.subarray.ndims - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), subsizes (");
                for (int j = 0; j < attr->u.subarray.ndims; j++) {
                    if (j >= 10 && j < attr->u.subarray.ndims - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.subarray.array_of_subsizes[j]);
                    if (j < attr->u.subarray.ndims - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), starts (");
                for (int j = 0; j < attr->u.subarray.ndims; j++) {
                    if (j >= 10 && j < attr->u.subarray.ndims - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.subarray.array_of_starts[j]);
                    if (j < attr->u.subarray.ndims - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), order ");
                if (attr->u.subarray.order == MPI_ORDER_C)
                    DTPI_snprintf(rc, desc, desclen, maxlen, "MPI_ORDER_C");
                else
                    DTPI_snprintf(rc, desc, desclen, maxlen, "MPI_ORDER_FORTRAN");
                break;

            case DTPI_DATATYPE_KIND__STRUCT:
                DTPI_snprintf(rc, desc, desclen, maxlen, "numblks %d, blklen (",
                              attr->u.structure.numblks);
                for (int j = 0; j < attr->u.structure.numblks; j++) {
                    if (j >= 10 && j < attr->u.structure.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%d",
                                  attr->u.structure.array_of_blklens[j]);
                    if (j < attr->u.structure.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, "), displs (");
                for (int j = 0; j < attr->u.structure.numblks; j++) {
                    if (j >= 10 && j < attr->u.structure.numblks - 10) {
                        if (j == 10) {
                            DTPI_snprintf(rc, desc, desclen, maxlen, "...,");
                        }
                        continue;
                    }
                    DTPI_snprintf(rc, desc, desclen, maxlen, "%zd",
                                  attr->u.structure.array_of_displs[j]);
                    if (j < attr->u.structure.numblks - 1)
                        DTPI_snprintf(rc, desc, desclen, maxlen, ",");
                }
                DTPI_snprintf(rc, desc, desclen, maxlen, ")");
                break;

            default:
                DTPI_ERR_ASSERT(0, rc);
        }

        DTPI_snprintf(rc, desc, desclen, maxlen, "]\n");
    }

    if (dtp->base_type_is_struct)
        DTPI_snprintf(rc, desc, desclen, maxlen, "base: struct of %s\n", dtp->base_type_str);
    else
        DTPI_snprintf(rc, desc, desclen, maxlen, "base: %s\n", dtp->base_type_str);

  fn_exit:
    *desc_ = desc;
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmip.h"
#include "utlist.h"

static int pmi_rank_to_local_id(struct pmip_pg *pg, int pmi_rank)
{
    for (int i = 0; i < pg->num_procs; i++) {
        if (pg->downstreams[i].pmi_rank == pmi_rank) {
            return i;
        }
    }
    return -1;
}

static struct pmip_barrier_epoch *PMIP_barrier_new_epoch(struct pmip_barrier *barrier)
{
    struct pmip_barrier_epoch *epoch;
    epoch = MPL_malloc(sizeof(*epoch), MPL_MEM_OTHER);
    if (epoch) {
        epoch->count = 0;
        epoch->proc_hash = NULL;
        DL_APPEND(barrier->epochs, epoch);
    }
    return epoch;
}

HYD_status PMIP_barrier_create(struct pmip_pg * pg, const char *group_str, int proc,
                               struct pmip_barrier ** barrier_out,
                               struct pmip_barrier_epoch ** epoch_out)
{

    HYD_status status = HYD_SUCCESS;

    /* first, survey group_str */
    const char *tail = NULL;
    int num_commas = 0;
    for (const char *s = group_str; *s; s++) {
        if (*s == ',') {
            num_commas++;
        } else if (*s == ':') {
            tail = s;
            break;
        }
    }

    int num_procs;              /* number of local processes */
    int total_count;            /* total number or processes in this barrier */

    int *proc_list;
    if (num_commas == 0) {
        /* check special names for PMIP_GROUP_ALL. Proxies need further distinguish them */
        if (strncmp(group_str, "WORLD", 5) == 0) {
            proc_list = PMIP_GROUP_ALL;
            num_procs = pg->num_procs;
            total_count = pg->global_process_count;
        } else if (strncmp(group_str, "NODE", 4) == 0) {
            proc_list = PMIP_GROUP_ALL;
            num_procs = pg->num_procs;
            total_count = pg->num_procs;
        } else {
            HYDU_ASSERT(0, status);
        }
    } else {
        /* general case: a comma-separated integer list */
        total_count = num_commas + 1;
        proc_list = MPL_malloc(MPL_MIN(pg->num_procs, total_count) * sizeof(int), MPL_MEM_OTHER);

        int i = 0;
        const char *str = group_str;
        while (*str && str != tail) {
            int pmi_rank = atoi(str);
            int local_id = pmi_rank_to_local_id(pg, pmi_rank);
            if (local_id != -1) {
                proc_list[i++] = local_id;
            }
            /* skip to next comma-separated id */
            while (*str && *str != ',' && str != tail) {
                str++;
            }
            if (*str == ',') {
                str++;
            } else {
                break;
            }
        }
        num_procs = i;
    }

    struct pmip_barrier *s;
    s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
    HYDU_ASSERT(s, status);

    /* fill the struct */
    s->name = MPL_strdup(group_str);
    s->num_procs = num_procs;
    s->total_count = total_count;
    s->proc_list = proc_list;
    s->stage = PMIP_BARRIER_INCOMPLETE;

    if (tail) {
        s->tag = atoi(tail + 1);
    } else {
        s->tag = 0;
    }

    s->has_upstream = (num_procs < total_count);
    s->epochs = NULL;

    /* add to pg */
    HASH_ADD_KEYPTR(hh, pg->barriers, s->name, strlen(s->name), s, MPL_MEM_OTHER);

    *barrier_out = s;
    *epoch_out = PMIP_barrier_new_epoch(s);

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status PMIP_barrier_find(struct pmip_pg *pg, const char *name, int proc,
                             struct pmip_barrier **barrier_out,
                             struct pmip_barrier_epoch **epoch_out)
{
    HYD_status status = HYD_SUCCESS;

    struct pmip_barrier *barrier = NULL;
    HASH_FIND_STR(pg->barriers, name, barrier);
    if (proc == -1) {
        /* server reply */
        HYDU_ASSERT(barrier, status);

        *barrier_out = barrier;
        *epoch_out = barrier->epochs;
    } else if (barrier) {
        struct pmip_barrier_epoch *epoch = NULL, *t;
        /* find the 1st epoch that there isn't a pending `barrier_in` from `proc` */
        DL_FOREACH(barrier->epochs, t) {
            struct pmip_barrier_proc_hash *h;
            HASH_FIND_INT(t->proc_hash, &proc, h);
            if (!h) {
                epoch = t;
                break;
            }
        }
        /* create a new epoch if necessary */
        if (!epoch) {
            epoch = PMIP_barrier_new_epoch(barrier);
        }

        *barrier_out = barrier;
        *epoch_out = epoch;
    } else {
        *barrier_out = NULL;
        *epoch_out = NULL;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status PMIP_barrier_epoch_in(struct pmip_barrier *barrier, struct pmip_barrier_epoch *epoch,
                                 int proc)
{
    HYD_status status = HYD_SUCCESS;

    /* verify proc is in barrier->proc_list */
    bool in_group = false;
    if (barrier->proc_list == PMIP_GROUP_ALL) {
        in_group = true;
    } else {
        for (int i = 0; i < barrier->num_procs; i++) {
            if (barrier->proc_list[i] == proc) {
                in_group = true;
                break;
            }
        }
    }
    HYDU_ASSERT(in_group, status);

    struct pmip_barrier_proc_hash *h;
    h = MPL_malloc(sizeof(*h), MPL_MEM_OTHER);
    h->proc = proc;
    HASH_ADD_INT(epoch->proc_hash, proc, h, MPL_MEM_OTHER);

    epoch->count++;
    if (epoch->count == barrier->num_procs) {
        barrier->stage = PMIP_BARRIER_LOCAL_COMPLETE;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status PMIP_barrier_complete(struct pmip_pg *pg, struct pmip_barrier **barrier_p)
{
    HYD_status status = HYD_SUCCESS;
    struct pmip_barrier *barrier = *barrier_p;

    /* only the head epoc should complete */
    struct pmip_barrier_epoch *epoch = barrier->epochs;

    /* free the epoch */
    struct pmip_barrier_proc_hash *h, *tmp;
    HASH_ITER(hh, epoch->proc_hash, h, tmp) {
        HASH_DEL(epoch->proc_hash, h);
        MPL_free(h);
    }
    DL_DELETE(barrier->epochs, epoch);
    MPL_free(epoch);

    if (!barrier->epochs) {
        /* free the barrier */
        MPL_free((char *) barrier->name);
        MPL_free(barrier->proc_list);
        HASH_DEL(pg->barriers, barrier);
        MPL_free(barrier);
        *barrier_p = NULL;
    } else {
        epoch = barrier->epochs;
        if (epoch->count == barrier->num_procs) {
            barrier->stage = PMIP_BARRIER_LOCAL_COMPLETE;
        } else {
            barrier->stage = PMIP_BARRIER_INCOMPLETE;
        }
    }

    return status;
}

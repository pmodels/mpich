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

#include <assert.h>
#include "gurt/hash.h"
#include "ad_daos.h"

extern daos_handle_t daos_pool_oh;
static struct d_hash_table *hdl_hash;

static inline struct adio_daos_co_hdl *
hdl_obj(d_list_t *rlink)
{
	return container_of(rlink, struct adio_daos_co_hdl, co_entry);
}

static bool
key_cmp(struct d_hash_table *htable, d_list_t *rlink,
	const void *key, unsigned int ksize)
{
	struct adio_daos_co_hdl *hdl = hdl_obj(rlink);

	return (uuid_compare(hdl->co_uuid, key) == 0);
}

static void
rec_addref(struct d_hash_table *htable, d_list_t *rlink)
{
	hdl_obj(rlink)->co_ref++;
}

static bool
rec_decref(struct d_hash_table *htable, d_list_t *rlink)
{
	struct adio_daos_co_hdl *hdl = hdl_obj(rlink);

	assert(hdl->co_ref > 0);
	hdl->co_ref--;
	return (hdl->co_ref == 0);
}

static void
rec_free(struct d_hash_table *htable, d_list_t *rlink)
{
	struct adio_daos_co_hdl *hdl = hdl_obj(rlink);

	assert(d_hash_rec_unlinked(&hdl->co_entry));
	assert(hdl->co_ref == 0);

	daos_cont_close(hdl->co_coh, NULL);
	ADIOI_Free(hdl);
}

static d_hash_table_ops_t hdl_hash_ops = {
	.hop_key_cmp	= key_cmp,
	.hop_rec_addref	= rec_addref,
	.hop_rec_decref	= rec_decref,
	.hop_rec_free	= rec_free
};

int
adio_daos_coh_hash_init(void)
{
	return d_hash_table_create(0, 16, NULL, &hdl_hash_ops, &hdl_hash);
}

void
adio_daos_coh_hash_finalize(void)
{
        d_hash_table_destroy(hdl_hash, true /* force */);
}

static void
hdl_delete(struct adio_daos_co_hdl *hdl)
{
	bool deleted;

	deleted = d_hash_rec_delete(hdl_hash, hdl->co_uuid, sizeof(uuid_t));
}

struct adio_daos_co_hdl *
adio_daos_cont_lookup(const uuid_t uuid)
{
	d_list_t *rlink;

	rlink = d_hash_rec_find(hdl_hash, uuid, sizeof(uuid_t));
	if (rlink == NULL)
		return NULL;

	return hdl_obj(rlink);
}

void
adio_daos_cont_release(struct adio_daos_co_hdl *hdl)
{
	d_hash_rec_decref(hdl_hash, &hdl->co_entry);
}

int
adio_daos_cont_lookup_create(uuid_t uuid, bool create, struct adio_daos_co_hdl **hdl)
{
	struct adio_daos_co_hdl *co_hdl;
	int rc;

	co_hdl = adio_daos_cont_lookup(uuid);
	if (co_hdl != NULL) {
            *hdl = co_hdl;
            return 0;
	}

	co_hdl = (struct adio_daos_co_hdl *)ADIOI_Calloc
            (1, sizeof(struct adio_daos_co_hdl));
	if (co_hdl == NULL)
		return -1;

	uuid_copy(co_hdl->co_uuid, uuid);

	/* Try to open the DAOS container first (the parent directory) */
	rc = daos_cont_open(daos_pool_oh, uuid, DAOS_COO_RW, &co_hdl->co_coh,
                            NULL, NULL);
	/* If it fails with NOEXIST we can create it then reopen if create mode */
	if (rc == -DER_NONEXIST && create) {
		rc = daos_cont_create(daos_pool_oh, uuid, NULL);
		if (rc == 0)
			rc = daos_cont_open(daos_pool_oh, uuid, DAOS_COO_RW,
					    &co_hdl->co_coh, NULL, NULL);
	}

	if (rc != 0) {
		PRINT_MSG(stderr, "failed to open/create DAOS container (%d)\n", rc);
		goto free_coh;
	}

        rc = d_hash_rec_insert(hdl_hash, co_hdl->co_uuid, sizeof(uuid_t),
                               &co_hdl->co_entry, true);
	if (rc) {
		PRINT_MSG(stderr, "Failed to add co_hdl to hashtable\n", rc);
		goto err_cont;
	}

        d_hash_rec_addref(hdl_hash, &co_hdl->co_entry);
        *hdl = co_hdl;

	return 0;

err_cont:
	daos_cont_close(co_hdl->co_coh, NULL);
free_coh:
	ADIOI_Free(co_hdl);
	return rc;
}

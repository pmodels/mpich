/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef COLL_CSEL_H_INCLUDED
#define COLL_CSEL_H_INCLUDED

#include "json.h"

int MPIR_Csel_create_from_file(const char *json_file,
                               void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_create_from_buf(const char *json,
                              void *(*create_container) (struct json_object *), void **csel);
int MPIR_Csel_free(void *csel);
void *MPIR_Csel_search(void *csel, MPIR_Csel_coll_sig_s * coll_sig);

void *MPII_Create_container(struct json_object *obj);

#endif /* COLL_CSEL_H_INCLUDED */

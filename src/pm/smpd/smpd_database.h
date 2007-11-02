/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_DATABASE_H
#define SMPD_DATABASE_H

#define SMPD_DBS_SUCCESS          0
#define SMPD_DBS_FAIL            -1
#define SMPD_MAX_DBS_NAME_LEN     256
#define SMPD_MAX_DBS_KEY_LEN      256
#define SMPD_MAX_DBS_VALUE_LEN    4096

#if defined(__cplusplus)
extern "C" {
#endif

int smpd_dbs_init(void);
int smpd_dbs_finalize(void);
int smpd_dbs_create(char *name);
int smpd_dbs_create_name_in(char *name);
int smpd_dbs_destroy(const char *name);
int smpd_dbs_get(const char *name, const char *key, char *value);
int smpd_dbs_put(const char *name, const char *key, const char *value);
int smpd_dbs_delete(const char *name, const char *key);
int smpd_dbs_first(const char *name, char *key, char *value);
int smpd_dbs_next(const char *name, char *key, char *value);
int smpd_dbs_firstdb(char *name);
int smpd_dbs_nextdb(char *name);

#if defined(__cplusplus)
}
#endif

#endif

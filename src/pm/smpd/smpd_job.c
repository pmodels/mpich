/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

typedef struct smpd_job_key_list_t
{
    char key[SMPD_MAX_NAME_LENGTH];
    char username[SMPD_MAX_ACCOUNT_LENGTH];
    char domain[SMPD_MAX_ACCOUNT_LENGTH];
    char full_domain[SMPD_MAX_ACCOUNT_LENGTH];
    HANDLE user_handle;
    HANDLE job;
    struct smpd_job_key_list_t *next;
} smpd_job_key_list_t;

static smpd_job_key_list_t *list = NULL;

#undef FCNAME
#define FCNAME "job_key_exists"
static SMPD_BOOL job_key_exists(const char *key)
{
    smpd_job_key_list_t *iter;

    smpd_enter_fn(FCNAME);

    iter = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    /* key matches */
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_add_job_key"
int smpd_add_job_key(const char *key, const char *username, const char *domain, const char *full_domain)
{
    int error;
    smpd_job_key_list_t *node;

    smpd_enter_fn(FCNAME);

    if (job_key_exists(key/*, username, domain, full_domain*/))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    node = (smpd_job_key_list_t*)MPIU_Malloc(sizeof(smpd_job_key_list_t));
    if (node == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(node->key, key);
    node->domain[0] = '\0';
    node->full_domain[0] = '\0';
    if (domain == NULL && full_domain == NULL)
    {
	smpd_parse_account_domain(username, node->username, node->domain);
	node->full_domain[0] = '\0';
    }
    else
    {
	strcpy(node->username, username);
    }
    if (domain != NULL)
	strcpy(node->domain, domain);
    if (full_domain != NULL)
	strcpy(node->full_domain, full_domain);
    node->user_handle = INVALID_HANDLE_VALUE;
    node->job = CreateJobObject(NULL, NULL);
    if (node->job == NULL)
    {
	error = GetLastError();
	smpd_err_printf("CreateJobObject failed: %d\n", error);
	MPIU_Free(node);
	smpd_exit_fn(FCNAME);
	return error;
    }
    node->next = list;
    list = node;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_add_job_key"
int smpd_add_job_key_and_handle(const char *key, const char *username, const char *domain, const char *full_domain, HANDLE hUser)
{
    int error;
    smpd_job_key_list_t *node;

    smpd_enter_fn(FCNAME);

    if (job_key_exists(key/*, username, domain, full_domain*/))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    node = (smpd_job_key_list_t*)MPIU_Malloc(sizeof(smpd_job_key_list_t));
    if (node == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(node->key, key);
    node->domain[0] = '\0';
    node->full_domain[0] = '\0';
    if (domain == NULL && full_domain == NULL)
    {
	smpd_parse_account_domain(username, node->username, node->domain);
	node->full_domain[0] = '\0';
    }
    else
    {
	strcpy(node->username, username);
    }
    if (domain != NULL)
	strcpy(node->domain, domain);
    if (full_domain != NULL)
	strcpy(node->full_domain, full_domain);
    node->user_handle = hUser;
    node->job = CreateJobObject(NULL, NULL);
    if (node->job == NULL)
    {
	error = GetLastError();
	smpd_err_printf("CreateJobObject failed: %d\n", error);
	MPIU_Free(node);
	smpd_exit_fn(FCNAME);
	return error;
    }
    node->next = list;
    list = node;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_remove_job_key"
int smpd_remove_job_key(const char *key)
{
    smpd_job_key_list_t *iter, *trailer;

    smpd_enter_fn(FCNAME);

    iter = trailer = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    if (iter == list)
	    {
		list = list->next;
	    }
	    else
	    {
		trailer->next = iter->next;
	    }
	    if (iter->user_handle != INVALID_HANDLE_VALUE)
		CloseHandle(iter->user_handle);
	    if (iter->job != NULL && iter->job != INVALID_HANDLE_VALUE)
	    {
		TerminateJobObject(iter->job, (UINT)-1);
		CloseHandle(iter->job);
	    }
	    MPIU_Free(iter);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_associate_job_key"
int smpd_associate_job_key(const char *key, const char *username, const char *domain, const char *full_domain, HANDLE user_handle)
{
    smpd_job_key_list_t *iter;

    smpd_enter_fn(FCNAME);

    iter = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    /* key matches */
	    if (strcmp(iter->username, username) == 0)
	    {
		/* username matches */
		if ((domain != NULL && stricmp(domain, iter->domain) == 0) ||                /* domain matches */
		    (full_domain != NULL && stricmp(full_domain, iter->full_domain) == 0) || /* full domain name matches */
		    (iter->domain[0] == '\0' && iter->full_domain[0] == '\0'))               /* no domain to match */
		{
		    /* domain matches */
		    if (iter->user_handle == INVALID_HANDLE_VALUE)
		    {
			/* handle not already set */
			smpd_dbg_printf("associated user token: %s,%p\n", username, user_handle);
			iter->user_handle = user_handle;
			smpd_exit_fn(FCNAME);
			return SMPD_SUCCESS;
		    }
		}
	    }
	}
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_lookup_job_key"
int smpd_lookup_job_key(const char *key, const char *username, HANDLE *user_handle, HANDLE *job_handle)
{
    smpd_job_key_list_t *iter;
    char account[SMPD_MAX_ACCOUNT_LENGTH];
    char domain[SMPD_MAX_ACCOUNT_LENGTH];

    smpd_enter_fn(FCNAME);

    smpd_parse_account_domain(username, account, domain);

    iter = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    /* key matches */
	    if (stricmp(iter->username, account) == 0)
	    {
		/* username matches */
		if ((stricmp(domain, iter->domain) == 0) ||
		    (stricmp(domain, iter->full_domain) == 0) ||
		    (iter->domain[0] == '\0' && iter->full_domain[0] == '\0'))
		{
		    /* domain matches */
		    if (iter->user_handle != INVALID_HANDLE_VALUE)
		    {
			*user_handle = iter->user_handle;
			*job_handle = iter->job;
			smpd_exit_fn(FCNAME);
			return SMPD_SUCCESS;
		    }
		}
	    }
	}
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

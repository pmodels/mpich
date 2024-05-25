/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef DISABLE_PMIX

#include "pmi_config.h"

#include "pmix.h"
#include "pmi_util.h"
#include "pmi_wire.h"
#include "pmi_msg.h"
#include "pmi_common.h"

#include "mpl.h"

#define USE_WIRE_VER  PMIU_WIRE_V2

/* we will call PMIU_cmd_free_buf */
static const bool no_static = false;
static pmix_proc_t PMIx_proc;
static int PMIx_size;
static int appnum;

static bool cached_singinit_inuse;
static char *cached_singinit_key;

static const char *attribute_from_key(const char *key);
static char *value_to_wire(pmix_value_t * val);
static pmix_value_t *wire_to_value(const char *encoded_val);

pmix_status_t PMIx_Init(pmix_proc_t * proc, pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    /* Get the fd for PMI commands; if none, we're a singleton */
    bool do_handshake ATTRIBUTE((unused));
    pmi_errno = PMIU_get_pmi_fd(&PMI_fd, &do_handshake);
    PMIU_ERR_POP(pmi_errno);

    if (PMI_fd == -1) {
        /* Singleton init: Process not started with mpiexec */
        PMI_initialized = SINGLETON_INIT_BUT_NO_PM;

        return PMIX_ERR_UNREACH;
    }

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* Get the value of PMI_DEBUG from the environment if possible, since
     * we may have set it to help debug the setup process */
    char *p;
    p = getenv("PMI_DEBUG");
    if (p) {
        PMIU_verbose = atoi(p);
    }

    /* get rank from env */
    const char *s_pmiid;
    int pmiid = -1;
    s_pmiid = getenv("PMI_ID");
    if (!s_pmiid) {
        s_pmiid = getenv("PMI_RANK");
    }
    if (s_pmiid) {
        pmiid = atoi(s_pmiid);
    }
    PMIx_proc.rank = pmiid;

    /* do initial PMI init */
    PMIU_msg_set_query_init(&pmicmd, PMIU_WIRE_V1, no_static, 2, 0);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    int server_version, server_subversion;
    pmi_errno = PMIU_msg_get_response_init(&pmicmd, &server_version, &server_subversion);

    PMIU_cmd_free_buf(&pmicmd);

    /* do full init */
    PMIU_msg_set_query_fullinit(&pmicmd, USE_WIRE_VER, no_static, pmiid);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *spawner_jobid = NULL;
    int verbose;                /* unused */
    PMIU_msg_get_response_fullinit(&pmicmd, &pmiid, &PMIx_size, &appnum, &spawner_jobid, &verbose);
    PMIU_ERR_POP(pmi_errno);

    PMIU_cmd_free_buf(&pmicmd);

    /* get the kvsname aka the namespace */
    PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_KVSNAME, no_static);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *jid;
    pmi_errno = PMIU_msg_get_response_kvsname(&pmicmd, &jid);
    PMIU_ERR_POP(pmi_errno);

    MPL_strncpy(PMIx_proc.nspace, jid, PMIX_MAX_NSLEN + 1);
    PMIU_Set_rank_kvsname(PMI_rank, jid);

    if (!PMI_initialized) {
        PMI_initialized = NORMAL_INIT_WITH_PM;
    }

    *proc = PMIx_proc;

    /* TODO: add reference counting? */

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Finalize(const pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_FINALIZE, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        shutdown(PMI_fd, SHUT_RDWR);
        close(PMI_fd);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Abort(int status, const char msg[], pmix_proc_t procs[], size_t nprocs)
{
    PMIU_printf(1, "aborting job:\n%s\n", msg);

    struct PMIU_cmd pmicmd;
    PMIU_msg_set_query_abort(&pmicmd, USE_WIRE_VER, no_static, status, msg);

    /* ignoring return code, because we're exiting anyway */
    PMIU_cmd_send(PMI_fd, &pmicmd);

    PMIU_Exit(status);
    return PMIX_SUCCESS;
}

pmix_status_t PMIx_Put(pmix_scope_t scope, const char key[], pmix_value_t * val)
{
    int pmi_errno = PMIX_SUCCESS;

    /* This is a special hack to support singleton initialization */
    if (PMI_initialized == SINGLETON_INIT_BUT_NO_PM) {
        if (cached_singinit_inuse)
            return PMIX_ERROR;
        cached_singinit_key = MPL_strdup(key);
        /* FIXME: save copy of value */
        cached_singinit_inuse = true;
        return PMIX_SUCCESS;
    }

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    char *wire_value = value_to_wire(val);
    assert(wire_value);

    PMIU_msg_set_query_kvsput(&pmicmd, USE_WIRE_VER, no_static, key, wire_value);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    MPL_free(wire_value);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Commit(void)
{
    return PMIX_SUCCESS;
}

pmix_status_t PMIx_Fence(const pmix_proc_t procs[], size_t nprocs,
                         const pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    if (PMI_initialized > SINGLETON_INIT_BUT_NO_PM) {
        PMIU_msg_set_query(&pmicmd, USE_WIRE_VER, PMIU_CMD_BARRIER, no_static);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Get(const pmix_proc_t * proc, const char key[],
                       const pmix_info_t info[], size_t ninfo, pmix_value_t ** val)
{
    int pmi_errno = PMIX_SUCCESS;

    /* first check for information we have locally */
    if (!strcmp(key, PMIX_JOB_SIZE)) {
        *val = MPL_direct_malloc(sizeof(pmix_value_t));
        (*val)->type = PMIX_UINT32;
        (*val)->data.uint32 = PMIx_size;
        return PMIX_SUCCESS;
    }

    if (!strcmp(key, PMIX_APPNUM)) {
        *val = MPL_direct_malloc(sizeof(pmix_value_t));
        (*val)->type = PMIX_UINT32;
        (*val)->data.uint32 = appnum;
        return PMIX_SUCCESS;
    }

    /* if there is no PMI server, just return not found */
    if (PMI_initialized <= SINGLETON_INIT_BUT_NO_PM) {
        return PMIX_ERR_NOT_FOUND;
    }

    if (!strcmp(key, PMIX_PARENT_ID)) {
        return PMIX_ERR_NOT_FOUND;
    }

    struct PMIU_cmd pmicmd;
    PMIU_cmd_init_zero(&pmicmd);

    /* handle predefined attributes */
    const char *attr = attribute_from_key(key);
    if (attr != NULL) {
        PMIU_msg_set_query_get(&pmicmd, USE_WIRE_VER, no_static, NULL, attr);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);

        bool found;
        const char *tmp_val;
        if (pmi_errno == PMIU_SUCCESS) {
            pmi_errno = PMIU_msg_get_response_get(&pmicmd, &tmp_val, &found);
        }

        if (!pmi_errno && found) {
            *val = MPL_direct_malloc(sizeof(pmix_value_t));
            if (!strcmp(key, PMIX_UNIV_SIZE)) {
                (*val)->type = PMIX_UINT32;
                (*val)->data.uint32 = atoi(tmp_val);
            } else {
                (*val)->type = PMIX_STRING;
                (*val)->data.string = MPL_direct_strdup(tmp_val);
            }
        } else {
            pmi_errno = PMIX_ERR_NOT_FOUND;
        }

        PMIU_cmd_free_buf(&pmicmd);
        goto fn_exit;
    }

    const char *nspace = PMIx_proc.nspace;
    int srcid = -1;
    if (proc != NULL) {
        /* user-provided namespace might be the empty string, ignore it */
        if (strlen(proc->nspace) != 0) {
            nspace = proc->nspace;
        }
        srcid = proc->rank;
    }
    PMIU_msg_set_query_kvsget(&pmicmd, USE_WIRE_VER, no_static, nspace, srcid, key);

    pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    const char *tmp_val;
    bool found;
    pmi_errno = PMIU_msg_get_response_kvsget(&pmicmd, &tmp_val, &found);
    PMIU_ERR_POP(pmi_errno);

    if (found) {
        *val = wire_to_value(tmp_val);
    } else {
        pmi_errno = PMIX_ERR_NOT_FOUND;
    }

    PMIU_cmd_free_buf(&pmicmd);

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Info_load(pmix_info_t * info,
                             const char *key, const void *data, pmix_data_type_t type)
{
    MPL_strncpy(info->key, key, PMIX_MAX_KEYLEN + 1);
    info->value.type = type;
    if (type == PMIX_BOOL) {
        info->value.data.flag = *(bool *) data;
    }

    return PMIX_SUCCESS;
}

pmix_status_t PMIx_Publish(const pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    struct PMIU_cmd pmicmd;

    for (int i = 0; i < ninfo; i++) {
        assert(info[i].value.type == PMIX_STRING);
        PMIU_msg_set_query_publish(&pmicmd, USE_WIRE_VER, no_static, info[i].key,
                                   info[i].value.data.string);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Lookup(pmix_pdata_t data[], size_t ndata, const pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    struct PMIU_cmd pmicmd;

    for (int i = 0; i < ndata; i++) {
        PMIU_msg_set_query_lookup(&pmicmd, USE_WIRE_VER, no_static, data[i].key);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);

        const char *tmp_port;
        pmi_errno = PMIU_msg_get_response_lookup(&pmicmd, &tmp_port);

        data[i].value.type = PMIX_STRING;
        data[i].value.data.string = MPL_direct_strdup(tmp_port);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Unpublish(char **keys, const pmix_info_t info[], size_t ninfo)
{
    int pmi_errno = PMIX_SUCCESS;

    struct PMIU_cmd pmicmd;

    for (int i = 0; keys[i]; i++) {
        PMIU_msg_set_query_unpublish(&pmicmd, USE_WIRE_VER, no_static, keys[i]);

        pmi_errno = PMIU_cmd_get_response(PMI_fd, &pmicmd);
        PMIU_ERR_POP(pmi_errno);
    }

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

pmix_status_t PMIx_Resolve_peers(const char *nodename,
                                 const pmix_nspace_t nspace, pmix_proc_t ** procs, size_t * nprocs)
{
    assert(0);
    return PMIX_SUCCESS;
}

pmix_status_t PMIx_Resolve_nodes(const pmix_nspace_t nspace, char **nodelist)
{
    assert(0);
    return PMIX_SUCCESS;
}

const char *PMIx_Error_string(pmix_status_t status)
{
    return "ERROR";
}

pmix_status_t PMIx_Spawn(const pmix_info_t job_info[], size_t ninfo,
                         const pmix_app_t apps[], size_t napps, pmix_nspace_t nspace)
{
    assert(0);
    return PMIX_SUCCESS;
}

/* convert predefined keys/attributes to the server format */
static const char *attribute_from_key(const char *key)
{
    if (!strcmp(key, PMIX_UNIV_SIZE)) {
        return "universeSize";
    } else if (!strcmp(key, PMIX_ANL_MAP)) {
        return "PMI_process_mapping";
    } else if (!strncmp(key, "PMI_", 4) || !strncmp(key, "pmix.", 5)) {
        return key;
    }

    return NULL;
}

/* utilities for converting put/get values to/from wire format */
static bool is_indirect_type(pmix_data_type_t type)
{
    switch (type) {
        case PMIX_STRING:
        case PMIX_BYTE_OBJECT:
        case PMIX_PROC_INFO:
        case PMIX_DATA_ARRAY:
        case PMIX_ENVAR:
        case PMIX_COORD:
        case PMIX_TOPO:
        case PMIX_PROC_CPUSET:
        case PMIX_GEOMETRY:
        case PMIX_DEVICE_DIST:
        case PMIX_ENDPOINT:
        case PMIX_DATA_BUFFER:
        case PMIX_PROC_NSPACE:
        case PMIX_PROC:
            return true;
    }

    return false;
}

static char *value_to_wire(pmix_value_t * val)
{
    char *encoded_value = NULL;
    void *data = NULL;
    int len = 0;

    if (is_indirect_type(val->type)) {
        if (val->type == PMIX_STRING) {
            data = val->data.string;
            len = strlen(val->data.string);
        } else if (val->type == PMIX_BYTE_OBJECT) {
            data = val->data.bo.bytes;
            len = val->data.bo.size;
        } else {
            /* TODO: handle other indirect types */
            assert(0);
        }
    }

    int size = sizeof(*val) * 2 + 1 /* delim */  + len * 2 + 1 /* nul */ ;
    encoded_value = MPL_malloc(size, MPL_MEM_OTHER);
    if (encoded_value == NULL) {
        return encoded_value;
    }

    /* Always encode the whole pmix_value_t struct, which contains the
     * type information and any value stored directly in the data member. */
    int rc;
    int len_out;
    rc = MPL_hex_encode(val, sizeof(*val), encoded_value, size, &len_out);
    PMIU_Assert(rc == MPL_SUCCESS);

    if (data != NULL) {
        /* Add deliminator '#' */
        encoded_value[len_out] = '#';
        char *encoded_data = encoded_value + (len_out + 1);
        size -= (len_out + 1);
        /* Add indirect data after the encoded pmix_value_t */
        rc = MPL_hex_encode(data, len, encoded_data, size, &len_out);
        PMIU_Assert(rc == MPL_SUCCESS);
    }

    return encoded_value;
}

static pmix_value_t *wire_to_value(const char *value)
{
    int rc;
    int len_out;

    pmix_value_t *decoded_value = MPL_direct_malloc(sizeof(pmix_value_t));
    rc = MPL_hex_decode(value, decoded_value, sizeof(pmix_value_t), &len_out);
    PMIU_Assert(rc == MPL_SUCCESS);

    pmix_data_type_t type;
    type = decoded_value->type;
    if (is_indirect_type(type)) {
        const char *s = value;
        while (*s && *s != '#') {
            s++;
        }
        PMIU_Assert(*s == '#');
        s++;

        if (type == PMIX_STRING) {
            int size = MPL_hex_decode_len(s);
            decoded_value->data.string = MPL_direct_malloc(size);
            rc = MPL_hex_decode(s, decoded_value->data.string, size, &len_out);
            PMIU_Assert(rc == MPL_SUCCESS && len_out == size);
        } else if (type == PMIX_BYTE_OBJECT) {
            int size = decoded_value->data.bo.size;
            decoded_value->data.bo.bytes = MPL_direct_malloc(size);
            rc = MPL_hex_decode(s, decoded_value->data.bo.bytes, size, &len_out);
            PMIU_Assert(rc == MPL_SUCCESS && len_out == size);
        } else {
            /* TODO: handle other indirect types */
            PMIU_Assert(0);
        }
    }

    return decoded_value;
}

#endif /* DISABLE_PMIX */

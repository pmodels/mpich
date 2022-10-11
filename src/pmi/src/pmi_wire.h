/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI_WIRE_H_INCLUDED
#define PMI_WIRE_H_INCLUDED

#define PMIU_WIRE_V1        1
#define PMIU_WIRE_V2        2

#define MAX_PMI_ARGS 1000
#define MAX_STATIC_PMI_ARGS 20

/* internal temporary buffers, used for PMIU_cmd_add_int etc. */
#define MAX_TOKEN_BUF_SIZE 50
#define MAX_STATIC_PMI_BUF_SIZE (MAX_PMI_ARGS * MAX_TOKEN_BUF_SIZE)

/* Internally a PMI command is represented by struct PMIU_cmd. It may result
 * from parsing a PMI command string, where buf points to the command string.
 * Or it may be constructed from scratch, where buf is not used. In either
 * case, the object does not allocate additional buffers.
 */
struct PMIU_token {
    const char *key;
    const char *val;
};

struct PMIU_cmd {
    bool buf_need_free;         /* if true, need call PMIU_cmd_free_buf to free buf */
    char *buf;                  /* buffer to hold the string before parsing */
    char *tmp_buf;              /* buffer to hold the serialization output */
    int version;                /* wire protocol: 1 or 2 */
    int cmd_id;                 /* id defined in pmi_msg.h */
    const char *cmd;
    struct PMIU_token *tokens;
    struct PMIU_token static_token_buf[MAX_STATIC_PMI_ARGS];
    int num_tokens;
};

#define CHECK_NUM_TOKENS(pmi) \
    do { \
        PMIU_Assert((pmi)->num_tokens < MAX_PMI_ARGS); \
        if (pmi->tokens == pmi->static_token_buf && pmi->num_tokens >= MAX_STATIC_PMI_ARGS) { \
            /* static pmi object cannot allocate memory */ \
            PMIU_Assert(!PMIU_cmd_is_static(pmi)); \
            pmi->tokens = MPL_malloc(MAX_PMI_ARGS * sizeof(struct PMIU_token), MPL_MEM_OTHER); \
            PMIU_Assert(pmi->tokens); \
            memcpy(pmi->tokens, pmi->static_token_buf, pmi->num_tokens * sizeof(struct PMIU_token)); \
        } \
    } while (0)

#define PMIU_CMD_ADD_TOKEN(pmi, k, v) \
    do { \
        int i_ = (pmi)->num_tokens; \
        pmicmd->tokens[i_].key = k; \
        pmicmd->tokens[i_].val = v; \
        (pmi)->num_tokens = i_ + 1; \
        CHECK_NUM_TOKENS(pmi); \
    } while (0)

/* set stack-allocated object to a sane state (rather than garbage) */
#define PMIU_cmd_init_zero(pmicmd) PMIU_cmd_init(pmicmd, 0, NULL)

/* Just parse the buf to get PMI command name. Do not alter buf. */
char *PMIU_wire_get_cmd(char *buf, int buflen, int pmi_version);
int PMIU_check_full_cmd(char *buf, int buflen, int *got_full_cmd,
                        int *cmdlen, int *version, int *cmd_id);
/* Construct MPII_pmi from parsing buf.
 * Note: buf will be modified during parsing.
 */
int PMIU_cmd_parse(char *buf, int buflen, int version, struct PMIU_cmd *pmicmd);

/* Construct MPII_pmi from scratch */
void PMIU_cmd_init(struct PMIU_cmd *pmicmd, int version, const char *cmd);
/* same as PMIU_cmd_init, but uses static internal buffer */
void PMIU_cmd_init_static(struct PMIU_cmd *pmicmd, int version, const char *cmd);
bool PMIU_cmd_is_static(struct PMIU_cmd *pmicmd);
void PMIU_cmd_add_token(struct PMIU_cmd *pmicmd, const char *token_str);
void PMIU_cmd_add_str(struct PMIU_cmd *pmicmd, const char *key, const char *val);
void PMIU_cmd_add_int(struct PMIU_cmd *pmicmd, const char *key, int val);
void PMIU_cmd_add_bool(struct PMIU_cmd *pmicmd, const char *key, int val);
/* for spawn, add keyval with keys such as "argv%d" */
void PMIU_cmd_add_substr(struct PMIU_cmd *pmicmd, const char *key, int i, const char *val);

void PMIU_cmd_free_buf(struct PMIU_cmd *pmicmd);
void PMIU_cmd_free(struct PMIU_cmd *pmicmd);
struct PMIU_cmd *PMIU_cmd_dup(struct PMIU_cmd *pmicmd);

void PMIU_cmd_get_tokens(struct PMIU_cmd *pmicmd,
                         int *num_tokens, const struct PMIU_token **tokens);
const char *PMIU_cmd_find_keyval(struct PMIU_cmd *pmicmd, const char *key);
const char *PMIU_cmd_find_keyval_segment(struct PMIU_cmd *pmi, const char *key, int segment_index);

#define PMIU_CMD_GET_STRVAL_WITH_DEFAULT(pmicmd, key, val, dfltval) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    if (tmp) { \
        val = tmp; \
    } else { \
        val = dfltval; \
    } \
} while (0)

#define PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmicmd, key, val, dfltval) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    if (tmp) { \
        val = atoi(tmp); \
    } else { \
        val = dfltval; \
    } \
} while (0)

#define PMIU_CMD_GET_BOOLVAL_WITH_DEFAULT(pmicmd, key, val, dfltval) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    if (tmp) { \
        if (strcmp(tmp, "TRUE") == 0) { \
            val = PMIU_TRUE; \
        } else if (strcmp(tmp, "FALSE") == 0) { \
            val = PMIU_FALSE; \
        } else { \
            val = dfltval; \
        } \
    } else { \
        val = dfltval; \
    } \
} while (0)

#define PMIU_CMD_GET_STRVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    val = tmp; \
} while (0)

#define PMIU_CMD_GET_INTVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    val = atoi(tmp); \
} while (0)

#define PMIU_CMD_GET_BOOLVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    if (strcmp(tmp, "TRUE") == 0) { \
        val = PMIU_TRUE; \
    } else if (strcmp(tmp, "FALSE") == 0) { \
        val = PMIU_FALSE; \
    } else { \
        val = -1; \
    } \
} while (0)

#define PMIU_CMD_EXPECT_STRVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    PMIU_ERR_CHKANDJUMP3(strcmp(tmp, val) != 0, pmi_errno, PMIU_FAIL, \
                         "Expect PMI response with %s=%s, got %s\n", key, val, tmp); \
} while (0)

#define PMIU_CMD_EXPECT_INTVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    PMIU_ERR_CHKANDJUMP3(atoi(tmp) != val, pmi_errno, PMIU_FAIL, \
                         "Expect PMI response with %s=%d, got %s\n", key, val, tmp); \
} while (0)

/* output to a string using a specific wire protocol */
int PMIU_cmd_output_v1(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);
int PMIU_cmd_output_v1_mcmd(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);
int PMIU_cmd_output_v1_initack(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);
int PMIU_cmd_output_v2(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);
/* output to a string based on embedded version in pmicmd */
int PMIU_cmd_output(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);

/* read and parse PMI command */
int PMIU_cmd_read(int fd, struct PMIU_cmd *pmicmd);

/* write PMI command to fd */
int PMIU_cmd_send(int fd, struct PMIU_cmd *pmicmd);

/* send a PMI command to fd and get a PMI response with expected cmd */
int PMIU_cmd_get_response(int fd, struct PMIU_cmd *pmicmd);

/* message layer utilities */
int PMIU_msg_cmd_to_id(const char *cmd);
const char *PMIU_msg_id_to_query(int version, int cmd_id);
const char *PMIU_msg_id_to_response(int version, int cmd_id);

void PMIU_msg_set_query(struct PMIU_cmd *pmi_query, int wire_version, int cmd_id, bool is_static);
int PMIU_msg_set_response(struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp, bool is_static);
int PMIU_msg_set_response_fail(struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp,
                               bool is_static, int rc, const char *error_message);

void PMIU_msg_set_query_spawn(struct PMIU_cmd *pmi_query, int version, bool is_static,
                              int count, const char *cmds[], const int maxprocs[],
                              int argcs[], const char **argvs[],
                              const int info_keyval_sizes[],
                              const struct PMIU_token *info_keyval_vectors[],
                              int preput_keyval_size,
                              const struct PMIU_token preput_keyval_vector[]);
int PMIU_msg_get_query_spawn_sizes(struct PMIU_cmd *pmi, int *count, int *total_args,
                                   int *total_info_keys, int *num_preput);
int PMIU_msg_get_query_spawn(struct PMIU_cmd *pmi, const char **cmds, int *maxprocs,
                             int *argcs, const char **argvs, int *info_counts,
                             struct PMIU_token *info_keyvals, struct PMIU_token *preput_keyvals);
#endif

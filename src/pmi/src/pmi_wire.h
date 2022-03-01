/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI_WIRE_H_INCLUDED
#define PMI_WIRE_H_INCLUDED

#define PMIU_WIRE_V1        1
#define PMIU_WIRE_V2        2
#define PMIU_WIRE_V1_MCMD   3

/* We may allocate stack arrays of size MAX_PMI_ARGS. Thus it shouldn't
 * be too big or result in stack-overflow. We assume a few kilobytes are safe.
 */
#define MAX_PMI_ARGS 1000

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
    const char *cmd;
    struct PMIU_token tokens[MAX_PMI_ARGS];
    int num_tokens;
};

/* Just parse the buf to get PMI command name. Do not alter buf. */
char *PMIU_wire_get_cmd(char *buf, int buflen, int pmi_version);
/* Construct MPII_pmi from parsing buf.
 * Note: buf will be modified during parsing.
 */
int PMIU_cmd_parse(char *buf, int buflen, int version, struct PMIU_cmd *pmicmd);

/* Construct MPII_pmi from scratch */
void PMIU_cmd_init(struct PMIU_cmd *pmicmd, int version, const char *cmd);
/* same as PMIU_cmd_init, but uses static internal buffer */
void PMIU_cmd_init_static(struct PMIU_cmd *pmicmd, int version, const char *cmd);
void PMIU_cmd_add_token(struct PMIU_cmd *pmicmd, const char *token_str);
void PMIU_cmd_add_str(struct PMIU_cmd *pmicmd, const char *key, const char *val);
void PMIU_cmd_add_int(struct PMIU_cmd *pmicmd, const char *key, int val);
void PMIU_cmd_add_bool(struct PMIU_cmd *pmicmd, const char *key, int val);
/* for spawn, add keyval with keys such as "argv%d" */
void PMIU_cmd_add_substr(struct PMIU_cmd *pmicmd, const char *key, int i, const char *val);

void PMIU_cmd_free_buf(struct PMIU_cmd *pmicmd);
void PMIU_cmd_free(struct PMIU_cmd *pmicmd);
struct PMIU_cmd *PMIU_cmd_dup(struct PMIU_cmd *pmicmd);

const char *PMIU_cmd_find_keyval(struct PMIU_cmd *pmicmd, const char *key);
const char *PMIU_cmd_find_keyval_segment(struct PMIU_cmd *pmi, const char *key,
                                         const char *segment_key, int segment_index);

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
int PMIU_cmd_output_v2(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);
/* output to a string based on embedded version in pmicmd */
int PMIU_cmd_output(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out);

/* read and parse PMI command */
int PMIU_cmd_read(int fd, struct PMIU_cmd *pmicmd);

/* write PMI command to fd */
int PMIU_cmd_send(int fd, struct PMIU_cmd *pmicmd);

/* send a PMI command to fd and get a PMI response with expected cmd */
int PMIU_cmd_get_response(int fd, struct PMIU_cmd *pmicmd, const char *expectedCmd);

#endif
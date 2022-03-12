/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef PMI_WIRE_H_INCLUDED
#define PMI_WIRE_H_INCLUDED

#define PMII_WIRE_V1        1
#define PMII_WIRE_V2        2
#define PMII_WIRE_V1_MCMD   3

/* We may allocate stack arrays of size MAX_PMI_ARGS. Thus it shouldn't
 * be too big or result in stack-overflow. We assume a few kilobytes are safe.
 */
#define MAX_PMI_ARGS 1000

/* Internally a PMI command is represented by struct PMIU_cmd. It may result
 * from parsing a PMI command string, where buf points to the command string.
 * Or it may be constructed from scratch, where buf is not used. In either
 * case, the object does not allocate additional buffers.
 */
struct PMII_token {
    const char *key;
    const char *val;
};

struct PMIU_cmd {
    char *buf;
    int len;
    int version;                /* wire protocol: 1 or 2 */
    const char *cmd;
    struct PMII_token tokens[MAX_PMI_ARGS];
    int num_tokens;
};

/* Construct MPII_pmi from parsing buf.
 * Note: buf will be modified during parsing.
 */
int PMIU_cmd_parse(char *buf, int buflen, int version, struct PMIU_cmd *pmicmd);

/* Construct MPII_pmi from scratch */
void PMIU_cmd_init(struct PMIU_cmd *pmicmd, int version, const char *cmd);
void PMIU_cmd_add_token(struct PMIU_cmd *pmicmd, const char *token_str);
void PMIU_cmd_add_str(struct PMIU_cmd *pmicmd, const char *key, const char *val);
void PMIU_cmd_add_int(struct PMIU_cmd *pmicmd, const char *key, int val);
void PMIU_cmd_add_bool(struct PMIU_cmd *pmicmd, const char *key, int val);
/* for spawn, add keyval with keys such as "argv%d" */
void PMIU_cmd_add_substr(struct PMIU_cmd *pmicmd, const char *key, int i, const char *val);

void PMIU_cmd_free_buf(struct PMIU_cmd *pmicmd);

const char *PMIU_cmd_find_keyval(struct PMIU_cmd *pmicmd, const char *key);
int PMIU_cmd_get_intval_with_default(struct PMIU_cmd *pmicmd, const char *key, int dfltval);

#define PMII_PMI_GET_STRVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    val = tmp; \
} while (0)

#define PMII_PMI_GET_INTVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    val = atoi(tmp); \
} while (0)

#define PMII_PMI_GET_BOOLVAL(pmicmd, key, val) do { \
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

#define PMII_PMI_EXPECT_STRVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    PMIU_ERR_CHKANDJUMP3(strcmp(tmp, val) != 0, pmi_errno, PMIU_FAIL, \
                         "Expect PMI response with %s=%s, got %s\n", key, val, tmp); \
} while (0)

#define PMII_PMI_EXPECT_INTVAL(pmicmd, key, val) do { \
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key); \
    PMIU_ERR_CHKANDJUMP1(tmp == NULL, pmi_errno, PMIU_FAIL, "PMI command missing key %s\n", key); \
    PMIU_ERR_CHKANDJUMP3(atoi(tmp) != val, pmi_errno, PMIU_FAIL, \
                         "Expect PMI response with %s=%d, got %s\n", key, val, tmp); \
} while (0)

/* read and parse PMI command */
int PMIU_cmd_read(int fd, struct PMIU_cmd *pmicmd);

/* write PMI command to fd */
int PMIU_cmd_send(int fd, struct PMIU_cmd *pmicmd);

/* send a PMI command to fd and get a PMI response with expected cmd */
int PMIU_cmd_get_response(int fd, struct PMIU_cmd *pmicmd, const char *expectedCmd);

#endif

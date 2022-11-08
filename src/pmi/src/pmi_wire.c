/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmi_config.h"
#include "mpl.h"
#include "pmi_util.h"
#include "pmi_common.h"
#include "pmi_wire.h"
#include "pmi_msg.h"

#include <ctype.h>

#define MAX_TMP_BUF_SIZE 1024
static char tmp_buf_for_output[MAX_TMP_BUF_SIZE];

#define IS_SPACE(c) ((c) == ' ')
#define IS_EOS(c) ((c) == '\0')
#define IS_EOL(c) ((c) == '\n' || (c) == '\0')
#define IS_KEY(c) (!(IS_SPACE(c) || IS_EOL(c) || (c) == '='))
#define IS_NONVAL(c) (IS_SPACE(c) || IS_EOL(c))

#define SKIP_SPACES(s) do { \
    while (IS_SPACE(*s)) s++; \
} while (0)

#define SKIP_EOL(s) do { \
    while (IS_SPACE(*s) || *s == '\n') s++; \
} while (0)

#define SKIP_KEY(s) do { \
    while (IS_KEY(*s)) s++; \
} while (0)

#define SKIP_VAL(s) do { \
    if (IS_NONVAL(*s)) { \
        break; \
    } else if (*s == '\\' && !IS_EOL(s[1])) { \
        s += 2; \
    } else { \
        s++; \
    } \
} while (1)

#define SKIP_VAL_MCMD(s) do { \
    if (IS_EOL(*s)) { \
        break; \
    } else { \
        s++; \
    } \
} while (1)

#define TERMINATE_STR(s) do { \
    if (*s) { \
        *s = '\0'; \
        s++; \
    } \
} while (0)

/* PMI-v1-mcmd wire protocol uses \n deliminator */
#define IS_NONVAL_MCMD(c) ((c) == '\n' || (c) == '\0')
/* PMI-v2 wire protocol uses ; deliminator */
#define IS_NONVAL_2(c) ((c) == ';' || (c) == '\0')

#define SKIP_VAL_2(s) do { \
    if (*s == '\0' || *s == ';') { \
        break; \
    } else { \
        s++; \
    } \
} while (1)

static void unescape_val(char *val)
{
    char *s1 = val;
    char *s2 = val;

    while (*s1 && *s1 != '\\')
        s1++;

    if (!*s1) {
        /* no escapes */
        return;
    }

    s2 = s1;
    while (*s2) {
        if (*s2 == '\\' && s2[1]) {
            *s1 = s2[1];
            s1++;
            s2 += 2;
        } else {
            *s1 = *s2;
            s1++;
            s2++;
        }
    }
    *s1 = '\0';
}

static int parse_v1(char *buf, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    char *p = buf;

    if (strncmp(buf, "cmd=", 4) != 0) {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, "Expecting cmd=");
    }

    while (1) {
        char *key = NULL;
        char *val = NULL;

        SKIP_SPACES(p);
        if (IS_EOL(*p)) {
            break;
        }
        /* expect key */
        if (IS_KEY(*p)) {
            key = p;
        } else {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Expecting key, got %c", *p);
        }
        SKIP_KEY(p);
        if (*p && *p != '=' && !IS_SPACE(*p) && !IS_EOL(*p)) {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Invalid char after key, got %c", *p);
        }

        /* expect =value or space or EOL */
        if (*p == '=') {
            TERMINATE_STR(p);   /* terminate key */
            if (IS_NONVAL(*p)) {
                PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, "Expecting value after =");
            }
            val = p;
            SKIP_VAL(p);
            TERMINATE_STR(p);   /* terminate value */
        } else {
            TERMINATE_STR(p);   /* terminate key */
        }

        if (val) {
            unescape_val(val);
        }

        if (strcmp(key, "cmd") == 0) {
            pmicmd->cmd = val;
        } else {
            PMIU_CMD_ADD_TOKEN(pmicmd, key, val);
        }
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int parse_v1_mcmd(char *buf, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    char *p = buf;

    if (strncmp(buf, "mcmd=spawn", 10) != 0) {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, "Expecting cmd=spawn");
    }
    pmicmd->cmd = "spawn";

    while (1) {
        char *key = NULL;
        char *val = NULL;

        SKIP_EOL(p);
        if (IS_EOS(*p)) {
            break;
        }
        /* expect key */
        if (IS_KEY(*p)) {
            key = p;
        } else {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Expecting key, got %c", *p);
        }
        SKIP_KEY(p);
        if (*p && *p != '=' && !IS_SPACE(*p) && !IS_EOL(*p)) {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Invalid char after key, got %c", *p);
        }

        /* expect =value or space or EOL */
        if (*p == '=') {
            TERMINATE_STR(p);   /* terminate key */
            if (IS_NONVAL_MCMD(*p)) {
                PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Expecting value after %s=", key);
            }
            val = p;
            /* value in mcmd can contain anything except '\n' and '\0' */
            SKIP_VAL_MCMD(p);
            TERMINATE_STR(p);   /* terminate value */
        } else {
            TERMINATE_STR(p);   /* terminate key */
        }

        if (val) {
            unescape_val(val);
        }

        /* turn the mcmd=spawn into NULL token, function as segment divider */
        if (strcmp(key, "mcmd") == 0) {
            key = NULL;
            val = NULL;
        }

        PMIU_CMD_ADD_TOKEN(pmicmd, key, val);
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int parse_v2(char *buf, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    char *p = buf + 6;          /* first 6 chars is length */

    if (strncmp(p, "cmd=", 4) != 0) {
        PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, "Expecting cmd=");
    }

    while (1) {
        char *key = NULL;
        char *val = NULL;

        SKIP_SPACES(p);
        if (IS_EOL(*p)) {
            break;
        }
        /* expect key */
        if (IS_KEY(*p)) {
            key = p;
        } else {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Expecting key, got %c", *p);
        }
        SKIP_KEY(p);
        if (*p && *p != '=' && !IS_SPACE(*p) && !IS_EOL(*p)) {
            PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "Invalid char after key, got %c", *p);
        }

        /* expect =value or space or EOL */
        if (*p == '=') {
            TERMINATE_STR(p);   /* terminate key */
            if (IS_NONVAL_2(*p)) {
                PMIU_ERR_SETANDJUMP(pmi_errno, PMIU_FAIL, "Expecting value after =");
            }
            val = p;
            SKIP_VAL_2(p);
            TERMINATE_STR(p);   /* terminate value */
        } else {
            TERMINATE_STR(p);   /* terminate key */
        }

        /* no escapes needed with PMI-v2 */

        if (strcmp(key, "cmd") == 0) {
            pmicmd->cmd = val;
        } else {
            if (strcmp(key, "subcmd") == 0) {
                /* insert segment divider */
                PMIU_CMD_ADD_TOKEN(pmicmd, NULL, NULL);
            }
            PMIU_CMD_ADD_TOKEN(pmicmd, key, val);
        }
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Construct PMIU_cmd from parsing buf */
int PMIU_cmd_parse(char *buf, int buflen, int version, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    PMIU_cmd_init(pmicmd, version, NULL);
    pmicmd->buf = buf;

    if (version == PMIU_WIRE_V1) {
        if (strncmp(buf, "mcmd=", 5) == 0) {
            pmi_errno = parse_v1_mcmd(buf, pmicmd);
        } else {
            pmi_errno = parse_v1(buf, pmicmd);
        }
    } else {
        /* PMIU_WIRE_V2 */
        pmi_errno = parse_v2(buf, pmicmd);
    }
    PMIU_ERR_POP(pmi_errno);

    pmicmd->cmd_id = PMIU_msg_cmd_to_id(pmicmd->cmd);

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Just parse the buf to get PMI command name. Do not alter buf. */
char *PMIU_wire_get_cmd(char *buf, int buflen, int pmi_version)
{
    static char cmd[100];       /* sufficient for PMI command name */

    char *s;
    if (pmi_version == 1) {
        if (strncmp(buf, "cmd=", 4) == 0) {
            s = buf + 4;
        } else if (strncmp(buf, "mcmd=", 5) == 0) {
            s = buf + 5;
        } else {
            return NULL;
        }
    } else {    /* PMI-v2 */
        if (strncmp(buf + 6, "cmd=", 4) == 0) {
            s = buf + 10;
        } else {
            return NULL;
        }
    }

    int i = 0;
    while (isalpha(s[i]) || s[i] == '-' || s[i] == '_') {
        i++;
    }
    assert(i < 100);

    strncpy(cmd, s, i);
    cmd[i] = '\0';
    return cmd;
}

/* Check whether we have a full command. */

static void copy_word(char *dest, const char *src, int n)
{
    int i;
    for (i = 0; i < n - 1; i++) {
        if (IS_KEY(src[i])) {
            dest[i] = src[i];
        } else {
            break;
        }
    }
    dest[i] = '\0';
}

int PMIU_check_full_cmd(char *buf, int buflen, int *got_full_cmd,
                        int *cmdlen, int *version, int *cmd_id)
{
    int pmi_errno = PMIU_SUCCESS;

    /* Parse the string and if a full command is found, make sure that
     * cmdlen is the length of the buffer and NUL-terminated if necessary */
    *got_full_cmd = 0;
    char cmdbuf[100];
    cmdbuf[0] = '\0';
    if (!strncmp(buf, "cmd=", strlen("cmd=")) || !strncmp(buf, "mcmd=", strlen("mcmd="))) {
        /* PMI-1 format command; read the rest of it */
        *version = PMIU_WIRE_V1;

        if (!strncmp(buf, "cmd=", strlen("cmd="))) {
            /* A newline marks the end of the command */
            char *bufptr;
            for (bufptr = buf; bufptr < buf + buflen; bufptr++) {
                if (*bufptr == '\n') {
                    *got_full_cmd = 1;
                    *bufptr = '\0';
                    *cmdlen = bufptr - buf + 1;
                    /* cmd= */
                    copy_word(cmdbuf, buf + 4, 100);
                    break;
                }
            }
        } else {        /* multi commands */
            /* TODO: assert mcmd=spawn */
            char *bufptr;
            int totspawns = 0, spawnssofar = 0;
            for (bufptr = buf; bufptr < buf + buflen - strlen("endcmd\n") + 1; bufptr++) {
                if (strncmp(bufptr, "totspawns=", 10) == 0) {
                    totspawns = atoi(bufptr + 10);
                } else if (strncmp(bufptr, "spawnssofar=", 12) == 0) {
                    spawnssofar = atoi(bufptr + 12);
                } else if (strncmp(bufptr, "endcmd\n", 7) == 0 && spawnssofar == totspawns) {
                    *got_full_cmd = 1;
                    bufptr += strlen("endcmd\n") - 1;
                    *bufptr = '\0';
                    *cmdlen = bufptr - buf + 1;
                    /* mcmd= */
                    copy_word(cmdbuf, buf + 5, 100);
                    break;
                }
            }
        }
    } else {
        *version = PMIU_WIRE_V2;

        /* We already made sure we had at least 6 bytes */
        char lenptr[7];
        memcpy(lenptr, buf, 6);
        lenptr[6] = 0;
        int len = atoi(lenptr);

        if (buflen >= len + 6) {
            *got_full_cmd = 1;
            char *bufptr = buf + 6 + len - 1;
            *bufptr = '\0';
            *cmdlen = len + 6;
            /* ------cmd= */
            copy_word(cmdbuf, buf + 10, 100);
        }
    }
    *cmd_id = PMIU_msg_cmd_to_id(cmdbuf);
    return pmi_errno;
}

/* Construct MPII_pmi from scratch */
void PMIU_cmd_init(struct PMIU_cmd *pmicmd, int version, const char *cmd)
{
    pmicmd->buf_need_free = false;
    pmicmd->buf = NULL;
    pmicmd->tmp_buf = NULL;
    pmicmd->version = version;
    pmicmd->cmd = cmd;
    pmicmd->num_tokens = 0;
    pmicmd->tokens = pmicmd->static_token_buf;
}

void PMIU_cmd_free_buf(struct PMIU_cmd *pmicmd)
{
    if (pmicmd->buf_need_free) {
        MPL_free(pmicmd->buf);
    }
    if (pmicmd->tokens != pmicmd->static_token_buf) {
        MPL_free(pmicmd->tokens);
        pmicmd->tokens = pmicmd->static_token_buf;
    }
    if (pmicmd->tmp_buf && pmicmd->tmp_buf != tmp_buf_for_output) {
        MPL_free(pmicmd->tmp_buf);
    }
    pmicmd->buf = NULL;
    pmicmd->tmp_buf = NULL;
}

void PMIU_cmd_free(struct PMIU_cmd *pmicmd)
{
    PMIU_cmd_free_buf(pmicmd);
    MPL_free(pmicmd);
}

static void transfer_pmi(struct PMIU_cmd *from, struct PMIU_cmd *to)
{
    PMIU_Assert(from->num_tokens < MAX_STATIC_PMI_ARGS);
    PMIU_cmd_init_zero(to);
    to->buf = from->buf;
    to->version = from->version;
    to->cmd = from->cmd;
    to->num_tokens = from->num_tokens;
    for (int i = 0; i < to->num_tokens; i++) {
        to->tokens[i] = from->tokens[i];
    }

    from->buf = NULL;
}

void PMIU_cmd_add_str(struct PMIU_cmd *pmicmd, const char *key, const char *val)
{
    if (val) {
        PMIU_CMD_ADD_TOKEN(pmicmd, key, val);
    }
}

void PMIU_cmd_add_token(struct PMIU_cmd *pmicmd, const char *token_str)
{
    PMIU_CMD_ADD_TOKEN(pmicmd, token_str, NULL);
}

/* The following construction routine may need buffer. When needed, we'll
 * allocate pmicmd->buf with size of MAX_PMI_ARGS * MAX_TOKEN_BUF_SIZE. The
 * corresponding token will use pmicmd->buf + i * MAX_TOKEN_BUF_SIZE.
 *
 * We only use the buffer to construct the pmicmd object before PMIU_cmd_send.
 * The buffer will be freed by PMIU_cmd_send.
 */

/* Initialize with static buffers, thus obviate the need to call PMIU_cmd_free_buf.
 * Obviously it won't work for concurrent PMIU_cmd instances, but most PMIU_cmd
 * usages are not concurrent.
 */
static char static_pmi_buf[MAX_STATIC_PMI_BUF_SIZE];

void PMIU_cmd_init_static(struct PMIU_cmd *pmicmd, int version, const char *cmd)
{
    PMIU_cmd_init(pmicmd, version, cmd);
    pmicmd->buf = static_pmi_buf;;
}

bool PMIU_cmd_is_static(struct PMIU_cmd *pmicmd)
{
    return (pmicmd->buf == static_pmi_buf);
}

#define PMII_PMI_ALLOC(pmicmd) do { \
    if (pmicmd->buf == NULL) { \
        pmicmd->buf = MPL_malloc(MAX_STATIC_PMI_BUF_SIZE, MPL_MEM_OTHER); \
        PMIU_Assert(pmicmd->buf); \
        pmicmd->buf_need_free = true; \
    } \
} while (0)

#define PMII_PMI_TOKEN_BUF(pmicmd) ((char *) (pmicmd)->buf + (pmicmd)->num_tokens * MAX_TOKEN_BUF_SIZE)

void PMIU_cmd_add_substr(struct PMIU_cmd *pmicmd, const char *key, int idx, const char *val)
{
    /* FIXME: add assertions to ensure key fits in the static space. */

    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd);
    snprintf(s, MAX_TOKEN_BUF_SIZE, key, idx);
    PMIU_CMD_ADD_TOKEN(pmicmd, s, val);
}

void PMIU_cmd_add_int(struct PMIU_cmd *pmicmd, const char *key, int val)
{
    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd);
    snprintf(s, MAX_TOKEN_BUF_SIZE, "%d", val);
    PMIU_CMD_ADD_TOKEN(pmicmd, key, s);
}

/* Used internally in PMIU_cmd_send to add thrid for PMI-v2 */
static void pmi_add_thrid(struct PMIU_cmd *pmicmd)
{
    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd);
    snprintf(s, MAX_TOKEN_BUF_SIZE, "%p", pmicmd);
    PMIU_CMD_ADD_TOKEN(pmicmd, "thrid", s);
}

void PMIU_cmd_add_bool(struct PMIU_cmd *pmicmd, const char *key, int val)
{
    PMIU_CMD_ADD_TOKEN(pmicmd, key, (val ? "TRUE" : "FALSE"));
}

void PMIU_cmd_get_tokens(struct PMIU_cmd *pmicmd, int *num_tokens, const struct PMIU_token **tokens)
{
    *num_tokens = pmicmd->num_tokens;
    *tokens = pmicmd->tokens;
}

/* keyval look up */
const char *PMIU_cmd_find_keyval(struct PMIU_cmd *pmicmd, const char *key)
{
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        if (pmicmd->tokens[i].key == NULL) {
            continue;
        }
        if (strcmp(pmicmd->tokens[i].key, key) == 0) {
            return pmicmd->tokens[i].val;
        }
    }
    return NULL;
}

/* This is for parsing spawn command, which contains multiple segments
 * separated NULL token key  */
const char *PMIU_cmd_find_keyval_segment(struct PMIU_cmd *pmi, const char *key, int segment_index)
{
    int cur_segment = -1;
    for (int i = 0; i < pmi->num_tokens; i++) {
        /* a NULL token starts a new segment */
        if (pmi->tokens[i].key == NULL) {
            cur_segment++;
            continue;
        }
        if (segment_index == cur_segment) {
            if (!strcmp(pmi->tokens[i].key, key)) {
                return pmi->tokens[i].val;
            }
        }
    }

    return NULL;
}

/* duplicate a pmicmd (for the purpose of enqueue) */
struct PMIU_cmd *PMIU_cmd_dup(struct PMIU_cmd *pmicmd)
{
    struct PMIU_cmd *pmi_copy = MPL_malloc(sizeof(struct PMIU_cmd), MPL_MEM_OTHER);
    assert(pmi_copy);

    PMIU_cmd_init(pmi_copy, pmicmd->version, NULL);
    PMIU_Assert(pmicmd->num_tokens < MAX_STATIC_PMI_ARGS);
    pmi_copy->num_tokens = pmicmd->num_tokens;
    pmi_copy->cmd_id = pmicmd->cmd_id;

    /* calc buflen to accommodate all token strings */
    int buflen = 0;
    buflen += strlen(pmicmd->cmd) + 1;
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        buflen += strlen(pmicmd->tokens[i].key) + 1;
        buflen += strlen(pmicmd->tokens[i].val) + 1;
    }
    /* allocate the buffer */
    pmi_copy->buf = MPL_malloc(buflen, MPL_MEM_OTHER);
    assert(pmi_copy->buf);
    pmi_copy->buf_need_free = true;
    char *s = pmi_copy->buf;

    /* copy cmd and tokens */
    strcpy(s, pmicmd->cmd);
    pmi_copy->cmd = s;
    s += strlen(pmicmd->cmd) + 1;
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        strcpy(s, pmicmd->tokens[i].key);
        pmi_copy->tokens[i].key = s;
        s += strlen(pmicmd->tokens[i].key) + 1;

        strcpy(s, pmicmd->tokens[i].val);
        pmi_copy->tokens[i].val = s;
        s += strlen(pmicmd->tokens[i].val) + 1;
    }

    return pmi_copy;
}


/* allocate serialization tmp_buf. Note: as safety, add 1 extra for NULL-termination */
#define PMIU_CMD_ALLOC_TMP_BUF(pmi, len) \
    do { \
        if (pmi->tmp_buf && pmi->tmp_buf != tmp_buf_for_output) { \
                MPL_free(pmi->tmp_buf); \
        } \
        if (len + 1 <= MAX_TMP_BUF_SIZE) { \
            pmi->tmp_buf = tmp_buf_for_output; \
        } else { \
            /* static pmi object cannot allocate memory */ \
            PMIU_Assert(!PMIU_cmd_is_static(pmi)); \
            pmi->tmp_buf = MPL_malloc(len + 1, MPL_MEM_OTHER); \
            PMIU_Assert(pmi->tmp_buf); \
        } \
    } while (0)

/* serialization output */
int PMIU_cmd_output_v1(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
{
    int pmi_errno = PMIU_SUCCESS;

    /* first, get the buflen */
    int buflen = 0;
    buflen += strlen("cmd=");
    buflen += strlen(pmicmd->cmd);
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        buflen += 1;    /* space */
        buflen += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            buflen += 1;        /* = */
            buflen += strlen(pmicmd->tokens[i].val);
        }
    }
    buflen += 1;        /* \n */

    PMIU_CMD_ALLOC_TMP_BUF(pmicmd, buflen);

    /* fill the string */
    char *s;
    s = pmicmd->tmp_buf;

    strcpy(s, "cmd=");
    s += strlen("cmd=");
    strcpy(s, pmicmd->cmd);
    s += strlen(pmicmd->cmd);

    for (int i = 0; i < pmicmd->num_tokens; i++) {
        *s = ' ';
        s++;
        strcpy(s, pmicmd->tokens[i].key);
        s += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            *s = '=';
            s++;
            strcpy(s, pmicmd->tokens[i].val);
            s += strlen(pmicmd->tokens[i].val);
        }
    }
    *s++ = '\n';
    *s = '\0';
    assert(strlen(pmicmd->tmp_buf) == buflen);

    *buf_out = pmicmd->tmp_buf;
    *buflen_out = buflen;

    return pmi_errno;
}

int PMIU_cmd_output_v1_mcmd(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
{
    int pmi_errno = PMIU_SUCCESS;

    /* first, get the buflen */
    int buflen = 0;
    buflen += strlen("mcmd=");
    buflen += strlen(pmicmd->cmd);
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        buflen += 1;    /* \n */
        buflen += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            buflen += 1;        /* = */
            buflen += strlen(pmicmd->tokens[i].val);
        }
    }
    /* NOTE: endcmd is in tokens with NULL val */
    buflen++;   /* \n */

    PMIU_CMD_ALLOC_TMP_BUF(pmicmd, buflen);

    /* fill the string */
    char *s;
    s = pmicmd->tmp_buf;

    strcpy(s, "mcmd=");
    s += strlen("mcmd=");
    strcpy(s, pmicmd->cmd);
    s += strlen(pmicmd->cmd);

    for (int i = 0; i < pmicmd->num_tokens; i++) {
        *s = '\n';
        s++;
        strcpy(s, pmicmd->tokens[i].key);
        s += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            *s = '=';
            s++;
            strcpy(s, pmicmd->tokens[i].val);
            s += strlen(pmicmd->tokens[i].val);
        }
    }
    *s++ = '\n';
    *s = '\0';
    assert(strlen(pmicmd->tmp_buf) == buflen);

    *buf_out = pmicmd->tmp_buf;
    *buflen_out = buflen;

    return pmi_errno;
}

int PMIU_cmd_output_v1_initack(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
{
    int pmi_errno;

    /* this variation require 3 additional "set" cmd for size, rank, and debug */
    int size, rank, debug;
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmicmd, "size", size, -1);
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmicmd, "rank", rank, -1);
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmicmd, "debug", debug, 0);

    pmi_errno = PMIU_cmd_output_v1(pmicmd, buf_out, buflen_out);

    /* if we are setting size and rank, we'll do 3 extra commands */
    if (rank >= 0 && size >= 0) {
        char *s = *buf_out + (*buflen_out);
        int len = MAX_TMP_BUF_SIZE - (*buflen_out);
        MPL_snprintf(s, len, "cmd=set size=%d\ncmd=set rank=%d\ncmd=set debug=%d\n", size, rank,
                     debug);

        *buflen_out += strlen(s);
    }

    return pmi_errno;
}

int PMIU_cmd_output_v2(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
{
    int pmi_errno = PMIU_SUCCESS;

    /* first, get the buflen */
    int buflen = 6;             /* The first 6 chars are reserved for length */
    buflen += strlen("cmd=");
    buflen += strlen(pmicmd->cmd);
    buflen += 1;        /* ; */
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        buflen += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            buflen += 1;        /* = */
            buflen += strlen(pmicmd->tokens[i].val);
        }
        buflen += 1;    /* ; */
    }

    PMIU_CMD_ALLOC_TMP_BUF(pmicmd, buflen);

    /* fill the string */
    char *s;
    s = pmicmd->tmp_buf;

    MPL_snprintf(s, 7, "%6u", buflen - 6);
    s += 6;

    strcpy(s, "cmd=");
    s += strlen("cmd=");
    strcpy(s, pmicmd->cmd);
    s += strlen(pmicmd->cmd);
    *s = ';';
    s++;

    for (int i = 0; i < pmicmd->num_tokens; i++) {
        strcpy(s, pmicmd->tokens[i].key);
        s += strlen(pmicmd->tokens[i].key);
        if (pmicmd->tokens[i].val) {
            *s = '=';
            s++;
            strcpy(s, pmicmd->tokens[i].val);
            s += strlen(pmicmd->tokens[i].val);
        }
        *s = ';';
        s++;
    }
    *s = '\0';
    assert(strlen(pmicmd->tmp_buf) == buflen);

    *buf_out = pmicmd->tmp_buf;
    *buflen_out = buflen;

    return pmi_errno;
}

int PMIU_cmd_output(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
{
    int pmi_errno = PMIU_SUCCESS;

    if (pmicmd->version == PMIU_WIRE_V1) {
        if (pmicmd->cmd_id == PMIU_CMD_SPAWN && strcmp(pmicmd->cmd, "spawn") == 0) {
            pmi_errno = PMIU_cmd_output_v1_mcmd(pmicmd, buf_out, buflen_out);
        } else if (pmicmd->cmd_id == PMIU_CMD_FULLINIT) {
            pmi_errno = PMIU_cmd_output_v1_initack(pmicmd, buf_out, buflen_out);
        } else {
            pmi_errno = PMIU_cmd_output_v1(pmicmd, buf_out, buflen_out);
        }
    } else {
        /* PMIU_WIRE_V2 */
        if (PMIU_is_threaded) {
            pmi_add_thrid(pmicmd);
        }
        pmi_errno = PMIU_cmd_output_v2(pmicmd, buf_out, buflen_out);
    }

    return pmi_errno;
}

int PMIU_cmd_read(int fd, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    PMIU_CS_ENTER;

    pmicmd->buf = NULL;
    while (pmicmd->buf == NULL) {
        char *recvbuf;
        PMIU_CHK_MALLOC(recvbuf, char *, PMIU_MAXLINE, pmi_errno, PMIU_ERR_NOMEM, "recvbuf");

        int n;
        n = PMIU_readline(fd, recvbuf, PMIU_MAXLINE);
        PMIU_ERR_CHKANDJUMP(n <= 0, pmi_errno, PMIU_FAIL, "readline failed\n");

        if (recvbuf[n - 1] == '\n') {
            PMIU_printf(PMIU_verbose, "got pmi response: %s", recvbuf);
        } else {
            PMIU_printf(PMIU_verbose, "got pmi response: %s\n", recvbuf);
        }

        if (strncmp(recvbuf, "cmd=", 4) == 0) {
            pmi_errno = PMIU_cmd_parse(recvbuf, strlen(recvbuf), PMIU_WIRE_V1, pmicmd);
        } else {
            pmi_errno = PMIU_cmd_parse(recvbuf, strlen(recvbuf), PMIU_WIRE_V2, pmicmd);
        }
        pmicmd->buf_need_free = true;
        PMIU_ERR_POP(pmi_errno);

        const char *thrid;
        thrid = PMIU_cmd_find_keyval(pmicmd, "thrid");
        if (thrid) {
            struct PMIU_cmd *cmd_pmi;
            int rc;
            rc = sscanf(thrid, "%p", &cmd_pmi);
            PMIU_ERR_CHKANDJUMP1(rc != 1, pmi_errno, PMIU_FAIL, "bad thrid (%s)\n", thrid);

            if (cmd_pmi != pmicmd) {
                /* transfer and reset pmicmd->buf to NULL */
                transfer_pmi(pmicmd, cmd_pmi);
            }
        }
    }

  fn_exit:
    PMIU_CS_EXIT;
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

int PMIU_cmd_send(int fd, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    PMIU_CS_ENTER;

    char *buf = NULL;
    int buflen = 0;

    PMIU_cmd_output(pmicmd, &buf, &buflen);

    if (buf[buflen - 1] == '\n') {
        PMIU_printf(PMIU_verbose, "send to fd=%d pmi: %s", fd, buf);
    } else {
        PMIU_printf(PMIU_verbose, "send to fd=%d pmi: %s\n", fd, buf);
    }

    pmi_errno = PMIU_write(fd, buf, buflen);
    PMIU_ERR_POP(pmi_errno);

    /* free the potential buffer that are used for constructing tokens */
    PMIU_cmd_free_buf(pmicmd);

  fn_exit:
    PMIU_CS_EXIT;
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * This function is used to request information from the server and check
 * that the response uses the expected command name.  On a successful
 * return from this routine, the response is parsed into the pmicmd object.
 * It can be queried for attributes.
 */
static int GetResponse_set_int(const char *key, int *val_out);

int PMIU_cmd_get_response(int fd, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    int cmd_id = pmicmd->cmd_id;
    const char *expectedCmd = PMIU_msg_id_to_response(pmicmd->version, cmd_id);
    PMIU_Assert(expectedCmd != NULL);

    pmi_errno = PMIU_cmd_send(fd, pmicmd);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIU_cmd_read(fd, pmicmd);
    PMIU_ERR_POP(pmi_errno);

    if (strcmp(expectedCmd, pmicmd->cmd) != 0) {
        PMIU_ERR_SETANDJUMP2(pmi_errno, PMIU_FAIL,
                             "expecting cmd=%s, got %s\n", expectedCmd, pmicmd->cmd);
    }

    /* check rc if included. */
    int rc;
    const char *msg;
    PMIU_CMD_GET_INTVAL_WITH_DEFAULT(pmicmd, "rc", rc, 0);
    if (rc) {
        PMIU_CMD_GET_STRVAL_WITH_DEFAULT(pmicmd, "msg", msg, NULL);
        if (!msg) {
            PMIU_CMD_GET_STRVAL_WITH_DEFAULT(pmicmd, "errmsg", msg, NULL);
        }
        PMIU_ERR_SETANDJUMP2(pmi_errno, PMIU_FAIL, "server responded with rc=%d - %s\n", rc, msg);
    }

    if (cmd_id == PMIU_CMD_FULLINIT && pmicmd->version == PMIU_WIRE_V1) {
        /* weird 3 additional set commands */
        pmi_errno = GetResponse_set_int("size", &PMI_size);
        PMIU_ERR_POP(pmi_errno);
        pmi_errno = GetResponse_set_int("rank", &PMI_rank);
        PMIU_ERR_POP(pmi_errno);
        pmi_errno = GetResponse_set_int("debug", &PMIU_verbose);
        PMIU_ERR_POP(pmi_errno);
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int GetResponse_set_int(const char *key, int *val_out)
{
    int pmi_errno = PMIU_SUCCESS;

    struct PMIU_cmd pmicmd;

    pmi_errno = PMIU_cmd_read(PMI_fd, &pmicmd);
    PMIU_ERR_POP(pmi_errno);

    if (strcmp("set", pmicmd.cmd) != 0) {
        PMIU_ERR_SETANDJUMP1(pmi_errno, PMIU_FAIL, "expecting cmd=set, got %s\n", pmicmd.cmd);
    }

    PMIU_CMD_GET_INTVAL(&pmicmd, key, *val_out);

  fn_exit:
    PMIU_cmd_free_buf(&pmicmd);
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* message layer utilities */

static void init_cmd(struct PMIU_cmd *pmi, int version, int cmd_id, bool is_static, bool is_query)
{
    const char *cmd;
    if (is_query) {
        cmd = PMIU_msg_id_to_query(version, cmd_id);
    } else {
        cmd = PMIU_msg_id_to_response(version, cmd_id);
    }

    if (is_static) {
        PMIU_cmd_init_static(pmi, version, cmd);
    } else {
        PMIU_cmd_init(pmi, version, cmd);
    }

    pmi->cmd_id = cmd_id;
}

void PMIU_msg_set_query(struct PMIU_cmd *pmi_query, int wire_version, int cmd_id, bool is_static)
{
    bool is_query = true;
    init_cmd(pmi_query, wire_version, cmd_id, is_static, is_query);
}

int PMIU_msg_set_response(struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp, bool is_static)
{
    bool is_query = false;
    init_cmd(pmi_resp, pmi_query->version, pmi_query->cmd_id, is_static, is_query);

    if (pmi_query->version == PMIU_WIRE_V2) {
        const char *thrid;
        thrid = PMIU_cmd_find_keyval(pmi_query, "thrid");
        if (thrid) {
            PMIU_cmd_add_str(pmi_resp, "thrid", thrid);
        }
    }

    PMIU_cmd_add_str(pmi_resp, "rc", "0");

    return PMIU_SUCCESS;
}

int PMIU_msg_set_response_fail(struct PMIU_cmd *pmi_query, struct PMIU_cmd *pmi_resp,
                               bool is_static, int rc, const char *error_message)
{
    bool is_query = false;
    init_cmd(pmi_resp, pmi_query->version, pmi_query->cmd_id, is_static, is_query);

    if (pmi_query->version == PMIU_WIRE_V2) {
        const char *thrid;
        thrid = PMIU_cmd_find_keyval(pmi_query, "thrid");
        if (thrid) {
            PMIU_cmd_add_str(pmi_resp, "thrid", thrid);
        }
    }

    PMIU_cmd_add_int(pmi_resp, "rc", rc);
    if (error_message) {
        if (pmi_query->version == PMIU_WIRE_V1) {
            PMIU_cmd_add_str(pmi_query, "msg", error_message);
        } else {
            /* PMIU_WIRE_V2 */
            PMIU_cmd_add_str(pmi_query, "errmsg", error_message);
        }
    }

    return PMIU_SUCCESS;
}

/* Spawn message functions (not generated in pmi_msg.c) */

void PMIU_msg_set_query_spawn(struct PMIU_cmd *pmi_query, int version, bool is_static,
                              int count, const char *cmds[], const int maxprocs[],
                              int argcs[], const char **argvs[],
                              const int info_keyval_sizes[],
                              const struct PMIU_token *info_keyval_vectors[],
                              int preput_keyval_size,
                              const struct PMIU_token preput_keyval_vector[])
{
    PMIU_msg_set_query(pmi_query, version, PMIU_CMD_SPAWN, is_static);
    if (version == PMIU_WIRE_V1) {
        for (int spawncnt = 0; spawncnt < count; spawncnt++) {
            if (spawncnt > 0) {
                /* Note: it is in fact multiple PMI commands */
                /* FIXME: use a proper separator token */
                PMIU_cmd_add_str(pmi_query, "mcmd", "spawn");
            }
            PMIU_cmd_add_int(pmi_query, "nprocs", maxprocs[spawncnt]);
            PMIU_cmd_add_str(pmi_query, "execname", cmds[spawncnt]);
            PMIU_cmd_add_int(pmi_query, "totspawns", count);
            PMIU_cmd_add_int(pmi_query, "spawnssofar", spawncnt + 1);

            PMIU_cmd_add_int(pmi_query, "argcnt", argcs[spawncnt]);
            for (int i = 0; i < argcs[spawncnt]; i++) {
                PMIU_cmd_add_substr(pmi_query, "arg%d", i + 1, argvs[spawncnt][i]);
            }

            if (spawncnt == 0) {
                PMIU_cmd_add_int(pmi_query, "preput_num", preput_keyval_size);
                for (int i = 0; i < preput_keyval_size; i++) {
                    PMIU_cmd_add_substr(pmi_query, "preput_key_%d", i, preput_keyval_vector[i].key);
                    PMIU_cmd_add_substr(pmi_query, "preput_val_%d", i, preput_keyval_vector[i].val);
                }
            }

            PMIU_cmd_add_int(pmi_query, "info_num", info_keyval_sizes[spawncnt]);
            for (int i = 0; i < info_keyval_sizes[spawncnt]; i++) {
                PMIU_cmd_add_substr(pmi_query, "info_key_%d", i,
                                    info_keyval_vectors[spawncnt][i].key);
                PMIU_cmd_add_substr(pmi_query, "info_val_%d", i,
                                    info_keyval_vectors[spawncnt][i].val);
            }
            PMIU_cmd_add_token(pmi_query, "endcmd");
        }
    } else if (version == PMIU_WIRE_V2) {
        PMIU_cmd_add_int(pmi_query, "ncmds", count);

        PMIU_cmd_add_int(pmi_query, "preputcount", preput_keyval_size);
        for (int i = 0; i < preput_keyval_size; i++) {
            PMIU_cmd_add_substr(pmi_query, "ppkey%d", i, preput_keyval_vector[i].key);
            PMIU_cmd_add_substr(pmi_query, "ppval%d", i, preput_keyval_vector[i].val);
        }

        for (int spawncnt = 0; spawncnt < count; spawncnt++) {
            PMIU_cmd_add_str(pmi_query, "subcmd", cmds[spawncnt]);
            PMIU_cmd_add_int(pmi_query, "maxprocs", maxprocs[spawncnt]);

            PMIU_cmd_add_int(pmi_query, "argc", argcs[spawncnt]);
            for (int i = 0; i < argcs[spawncnt]; i++) {
                PMIU_cmd_add_substr(pmi_query, "argv%d", i, argvs[spawncnt][i]);
            }

            PMIU_cmd_add_int(pmi_query, "infokeycount", info_keyval_sizes[spawncnt]);
            for (int i = 0; i < info_keyval_sizes[spawncnt]; i++) {
                PMIU_cmd_add_substr(pmi_query, "infokey%d", i,
                                    info_keyval_vectors[spawncnt][i].key);
                PMIU_cmd_add_substr(pmi_query, "infoval%d", i,
                                    info_keyval_vectors[spawncnt][i].val);
            }
        }
    } else {
        PMIU_Assert(0);
    }
}

/* macros to make the code less clutter */
#define KEYi pmi->tokens[i].key
#define VALi pmi->tokens[i].val

int PMIU_msg_get_query_spawn_sizes(struct PMIU_cmd *pmi, int *count, int *total_args,
                                   int *total_info_keys, int *num_preput)
{
    int pmi_errno = PMIU_SUCCESS;

    *count = 0;
    *num_preput = 0;
    *total_args = 0;
    *total_info_keys = 0;

    int i_seg = 0;
    for (int i = 0; i < pmi->num_tokens; i++) {
        if (KEYi == NULL) {
            i_seg++;
            continue;
        }
        if (pmi->version == PMIU_WIRE_V1) {
            if (i_seg == 1 && strcmp(KEYi, "totspawns") == 0) {
                *count = atoi(VALi);
            } else if (i_seg == 1 && strcmp(KEYi, "preput_num") == 0) {
                *num_preput = atoi(VALi);
            } else if (strcmp(KEYi, "argcnt") == 0) {
                *total_args += atoi(VALi);
            } else if (strcmp(KEYi, "info_num") == 0) {
                *total_info_keys += atoi(VALi);
            }
        } else if (pmi->version == PMIU_WIRE_V2) {
            if (strcmp(KEYi, "ncmds") == 0) {
                *count = atoi(VALi);
            } else if (strcmp(KEYi, "preputcount") == 0) {
                *num_preput = atoi(VALi);
            } else if (strcmp(KEYi, "argc") == 0) {
                *total_args += atoi(VALi);
            } else if (strcmp(KEYi, "infokeycount") == 0) {
                *total_info_keys += atoi(VALi);
            }
        }
    }

    return pmi_errno;
}

int PMIU_msg_get_query_spawn(struct PMIU_cmd *pmi, const char **cmds, int *maxprocs,
                             int *argcs, const char **argvs, int *info_counts,
                             struct PMIU_token *info_keyvals, struct PMIU_token *preput_keyvals)
{
    int pmi_errno = PMIU_SUCCESS;

    int count = 0;
    int num_preput = 0, total_args = 0, total_info_keys = 0;
    int i_cmd = 0, i_nprocs = 0, i_argc = 0, i_info_count = 0;
    int i_argv = 0, i_info_key = 0, i_info_val = 0, i_preput_key = 0, i_preput_val = 0;

    int i_seg = 0;
    for (int i = 0; i < pmi->num_tokens; i++) {
        if (KEYi == NULL) {
            i_seg++;
            continue;
        }
        if (pmi->version == PMIU_WIRE_V1) {
            if (strcmp(KEYi, "totspawns") == 0) {
                if (i_seg == 1) {
                    count = atoi(VALi);
                } else {
                    PMIU_ERR_POP(atoi(VALi) != count);
                }
            } else if (strcmp(KEYi, "spawnssofar") == 0) {
                PMIU_ERR_POP(atoi(VALi) != i_seg);
            } else if (strcmp(KEYi, "execname") == 0) {
                cmds[i_cmd++] = VALi;
                PMIU_ERR_POP(i_cmd != i_seg);
            } else if (strcmp(KEYi, "nprocs") == 0) {
                maxprocs[i_nprocs++] = atoi(VALi);
                PMIU_ERR_POP(i_nprocs != i_seg);
            } else if (strcmp(KEYi, "argcnt") == 0) {
                int val = atoi(VALi);
                argcs[i_argc++] = val;
                PMIU_ERR_POP(i_argc != i_seg);
                total_args += val;
            } else if (strncmp(KEYi, "arg", 3) == 0) {
                argvs[i_argv++] = VALi;
            } else if (i_seg == 1 && strcmp(KEYi, "preput_num") == 0) {
                num_preput = atoi(VALi);
            } else if (i_seg == 1 && strncmp(KEYi, "preput_key_", 10) == 0) {
                preput_keyvals[i_preput_key++].key = VALi;
            } else if (i_seg == 1 && strncmp(KEYi, "preput_val_", 10) == 0) {
                preput_keyvals[i_preput_val++].val = VALi;
            } else if (strcmp(KEYi, "info_num") == 0) {
                info_counts[i_info_count++] = atoi(VALi);
                PMIU_ERR_POP(i_info_count != i_seg);
            } else if (strncmp(KEYi, "info_key_", 8) == 0) {
                info_keyvals[i_info_key++].key = VALi;
            } else if (strncmp(KEYi, "info_val_", 8) == 0) {
                info_keyvals[i_info_val++].val = VALi;
            }
        } else if (pmi->version == PMIU_WIRE_V2) {
            if (i_seg == 0 && strcmp(KEYi, "ncmds") == 0) {
                count = atoi(VALi);
            } else if (strcmp(KEYi, "subcmd") == 0) {
                cmds[i_cmd++] = VALi;
                PMIU_ERR_POP(i_cmd != i_seg);
            } else if (strcmp(KEYi, "maxprocs") == 0) {
                maxprocs[i_nprocs++] = atoi(VALi);
                PMIU_ERR_POP(i_nprocs != i_seg);
            } else if (strcmp(KEYi, "argc") == 0) {
                int val = atoi(VALi);
                argcs[i_argc++] = val;
                PMIU_ERR_POP(i_argc != i_seg);
                total_args += val;
            } else if (strncmp(KEYi, "argv", 4) == 0) {
                argvs[i_argv++] = VALi;
            } else if (i_seg == 0 && strcmp(KEYi, "preputcount") == 0) {
                num_preput = atoi(VALi);
            } else if (i_seg == 0 && strncmp(KEYi, "ppkey", 5) == 0) {
                preput_keyvals[i_preput_key++].key = VALi;
            } else if (i_seg == 0 && strncmp(KEYi, "ppval", 5) == 0) {
                preput_keyvals[i_preput_val++].val = VALi;
            } else if (strcmp(KEYi, "infokeycount") == 0) {
                info_counts[i_info_count++] = atoi(VALi);
                PMIU_ERR_POP(i_info_count != i_seg);
            } else if (strncmp(KEYi, "infokey", 7) == 0) {
                info_keyvals[i_info_key++].key = VALi;
            } else if (strncmp(KEYi, "infoval_", 7) == 0) {
                info_keyvals[i_info_val++].val = VALi;
            }
        }
    }
    PMIU_ERR_POP(i_seg != count);
    PMIU_ERR_POP(i_cmd != count);
    PMIU_ERR_POP(i_nprocs != count);
    PMIU_ERR_POP(i_argc != count);
    PMIU_ERR_POP(i_info_count != count);
    PMIU_ERR_POP(i_argv != total_args);
    PMIU_ERR_POP(i_preput_val != num_preput || i_preput_key != num_preput);
    PMIU_ERR_POP(i_info_val != total_info_keys || i_info_key != total_info_keys);

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

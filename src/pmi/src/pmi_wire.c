/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmi_config.h"
#include "mpl.h"
#include "pmi_util.h"
#include "pmi_common.h"
#include "pmi_wire.h"

#include <ctype.h>

#define IS_SPACE(c) ((c) == ' ')
#define IS_EOL(c) ((c) == '\n' || (c) == '\0')
#define IS_KEY(c) (isalnum(c) || (c) == '_' || (c) == '-')
#define IS_NONVAL(c) (IS_SPACE(c) || IS_EOL(c))

#define SKIP_SPACES(s) do { \
    while (IS_SPACE(*s)) s++; \
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

#define TERMINATE_STR(s) do { \
    if (*s) { \
        *s = '\0'; \
        s++; \
    } \
} while (0)

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
    int idx = 0;

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
            pmicmd->tokens[idx].key = key;
            pmicmd->tokens[idx].val = val;
            idx++;
            PMIU_Assert(idx < MAX_PMI_ARGS);
        }
    }
    pmicmd->num_tokens = idx;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int parse_v2(char *buf, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    char *p = buf + 6;          /* first 6 chars is length */
    int idx = 0;

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
            pmicmd->tokens[idx].key = key;
            pmicmd->tokens[idx].val = val;
            idx++;
            PMIU_Assert(idx < MAX_PMI_ARGS);
        }
    }
    pmicmd->num_tokens = idx;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Construct PMIU_cmd from parsing buf */
int PMIU_cmd_parse(char *buf, int buflen, int version, struct PMIU_cmd *pmicmd)
{
    int pmi_errno = PMIU_SUCCESS;

    pmicmd->buf = buf;
    pmicmd->len = buflen;
    pmicmd->version = version;
    pmicmd->cmd = NULL;
    pmicmd->num_tokens = 0;

    if (version == PMII_WIRE_V1) {
        pmi_errno = parse_v1(buf, pmicmd);
    } else {
        /* PMII_WIRE_V2 */
        pmi_errno = parse_v2(buf, pmicmd);
    }
    PMIU_ERR_POP(pmi_errno);

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

/* Construct MPII_pmi from scratch */
void PMIU_cmd_init(struct PMIU_cmd *pmicmd, int version, const char *cmd)
{
    pmicmd->buf = NULL;
    pmicmd->version = version;
    pmicmd->cmd = cmd;
    pmicmd->num_tokens = 0;
}

void PMIU_cmd_free_buf(struct PMIU_cmd *pmicmd)
{
    MPL_free(pmicmd->buf);
    pmicmd->buf = NULL;
}

static void transfer_pmi(struct PMIU_cmd *from, struct PMIU_cmd *to)
{
    to->buf = from->buf;
    to->len = from->len;
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
        int i = pmicmd->num_tokens;
        pmicmd->tokens[i].key = key;
        pmicmd->tokens[i].val = val;
        pmicmd->num_tokens++;
    }
}

void PMIU_cmd_add_token(struct PMIU_cmd *pmicmd, const char *token_str)
{
    int i = pmicmd->num_tokens;
    pmicmd->tokens[i].key = token_str;
    pmicmd->tokens[i].val = NULL;
    pmicmd->num_tokens++;
}

/* The following construction routine may need buffer. When needed, we'll
 * allocate pmicmd->buf with size of MAX_PMI_ARGS * MAX_TOKEN_BUF_SIZE. The
 * corresponding token will use pmicmd->buf + i * MAX_TOKEN_BUF_SIZE.
 *
 * We only use the buffer to construct the pmicmd object before PMIU_cmd_send.
 * The buffer will be freed by PMIU_cmd_send.
 */
#define MAX_TOKEN_BUF_SIZE 50   /* We only use it for e.g. "%d", "%p", etc.
                                 * The longest may be "infokey%d" */
#define PMII_PMI_ALLOC(pmicmd) do { \
    if (pmicmd->buf == NULL) { \
        pmicmd->buf = MPL_malloc(MAX_PMI_ARGS * MAX_TOKEN_BUF_SIZE, MPL_MEM_OTHER); \
        PMIU_Assert(pmicmd->buf); \
    } \
} while (0)

#define PMII_PMI_TOKEN_BUF(pmicmd, i) ((char *) pmicmd->buf + i * MAX_TOKEN_BUF_SIZE)

void PMIU_cmd_add_substr(struct PMIU_cmd *pmicmd, const char *key, int idx, const char *val)
{
    /* FIXME: add assertions to ensure key fits in the static space. */

    int i = pmicmd->num_tokens;
    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd, i);
    snprintf(s, MAX_TOKEN_BUF_SIZE, key, idx);
    pmicmd->tokens[i].key = s;
    pmicmd->tokens[i].val = val;
    pmicmd->num_tokens++;
}

void PMIU_cmd_add_int(struct PMIU_cmd *pmicmd, const char *key, int val)
{
    int i = pmicmd->num_tokens;
    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd, i);
    snprintf(s, MAX_TOKEN_BUF_SIZE, "%d", val);
    pmicmd->tokens[i].key = key;
    pmicmd->tokens[i].val = s;
    pmicmd->num_tokens++;
}

/* Used internally in PMIU_cmd_send to add thrid for PMI-v2 */
static void pmi_add_thrid(struct PMIU_cmd *pmicmd)
{
    int i = pmicmd->num_tokens;
    PMII_PMI_ALLOC(pmicmd);
    char *s = PMII_PMI_TOKEN_BUF(pmicmd, i);
    snprintf(s, MAX_TOKEN_BUF_SIZE, "%p", pmicmd);
    pmicmd->tokens[i].key = "thrid";
    pmicmd->tokens[i].val = s;
    pmicmd->num_tokens++;
}

void PMIU_cmd_add_bool(struct PMIU_cmd *pmicmd, const char *key, int val)
{
    int i = pmicmd->num_tokens;
    pmicmd->tokens[i].key = key;
    pmicmd->tokens[i].val = val ? "TRUE" : "FALSE";
    pmicmd->num_tokens++;
}

/* keyval look up */
const char *PMIU_cmd_find_keyval(struct PMIU_cmd *pmicmd, const char *key)
{
    for (int i = 0; i < pmicmd->num_tokens; i++) {
        if (strcmp(pmicmd->tokens[i].key, key) == 0) {
            return pmicmd->tokens[i].val;
        }
    }
    return NULL;
}

int PMIU_cmd_get_intval_with_default(struct PMIU_cmd *pmicmd, const char *key, int dfltval)
{
    const char *tmp = PMIU_cmd_find_keyval(pmicmd, key);
    if (tmp) {
        return atoi(tmp);
    } else {
        return dfltval;
    }
}

/* serialization output */
static int output_pmi_v1(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
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

    /* allocate buffer. Note: as safety, add 1 extra for NULL-termination */
    char *buf;
    PMIU_CHK_MALLOC(buf, char *, buflen + 1, pmi_errno, PMIU_ERR_NOMEM, "buf");

    /* fill the string */
    char *s;
    s = buf;

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
    assert(strlen(buf) == buflen);

    *buf_out = buf;
    *buflen_out = buflen;

  fn_exit:
    return pmi_errno;

  fn_fail:
    goto fn_exit;
}

static int output_pmi_v1_mcmd(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
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

    /* allocate buffer. Note: as safety, add 1 extra for NULL-termination */
    char *buf;
    PMIU_CHK_MALLOC(buf, char *, buflen + 1, pmi_errno, PMIU_ERR_NOMEM, "buf");

    /* fill the string */
    char *s;
    s = buf;

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
    assert(strlen(buf) == buflen);

    *buf_out = buf;
    *buflen_out = buflen;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

static int output_pmi_v2(struct PMIU_cmd *pmicmd, char **buf_out, int *buflen_out)
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

    /* allocate buffer. Note: as safety, add 1 extra for NULL-termination */
    char *buf;
    PMIU_CHK_MALLOC(buf, char *, buflen + 1, pmi_errno, PMIU_ERR_NOMEM, "buf");

    /* fill the string */
    char *s;
    s = buf;

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
    assert(strlen(buf) == buflen);

    *buf_out = buf;
    *buflen_out = buflen;

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
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

        if (strncmp(recvbuf, "cmd=", 4) == 0) {
            pmi_errno = PMIU_cmd_parse(recvbuf, strlen(recvbuf), PMII_WIRE_V1, pmicmd);
        } else {
            pmi_errno = PMIU_cmd_parse(recvbuf, strlen(recvbuf), PMII_WIRE_V2, pmicmd);
        }
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

    if (pmicmd->version == PMII_WIRE_V1) {
        output_pmi_v1(pmicmd, &buf, &buflen);
    } else if (pmicmd->version == PMII_WIRE_V1_MCMD) {
        output_pmi_v1_mcmd(pmicmd, &buf, &buflen);
    } else {
        /* PMII_WIRE_V2 */
        if (PMIU_is_threaded) {
            pmi_add_thrid(pmicmd);
        }
        output_pmi_v2(pmicmd, &buf, &buflen);
    }

    pmi_errno = PMIU_write(fd, buf, buflen);
    PMIU_ERR_POP(pmi_errno);

    /* free the potential buffer that are used for constructing tokens */
    PMIU_cmd_free_buf(pmicmd);

  fn_exit:
    MPL_free(buf);
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
int PMIU_cmd_get_response(int fd, struct PMIU_cmd *pmicmd, const char *expectedCmd)
{
    int pmi_errno = PMIU_SUCCESS;

    pmi_errno = PMIU_cmd_send(fd, pmicmd);
    PMIU_ERR_POP(pmi_errno);

    pmi_errno = PMIU_cmd_read(fd, pmicmd);
    PMIU_ERR_POP(pmi_errno);

    if (strcmp(expectedCmd, pmicmd->cmd) != 0) {
        PMIU_ERR_SETANDJUMP2(pmi_errno, PMIU_FAIL,
                             "expecting cmd=%s, got %s\n", expectedCmd, pmicmd->cmd);
    }

  fn_exit:
    return pmi_errno;
  fn_fail:
    goto fn_exit;
}

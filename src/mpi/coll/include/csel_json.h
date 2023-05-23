/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CSEL_JSON_H_INCLUDED
#define CSEL_JSON_H_INCLUDED

struct json_stream {
    const char *json_str;
    int pos;
};

#define JSON_STREAM_INIT(json_stream, str)  \
    do { \
        (json_stream)->json_str = str; \
        (json_stream)->pos = 0; \
    } while (0)

#define JSON_STREAM_GET_S(json_stream, s)  \
    do { \
        s = (json_stream)->json_str + (json_stream)->pos; \
    } while (0)

#define JSON_STREAM_UPDATE_POS(json_stream, s) \
    do { \
        (json_stream->pos) = (s) - (json_stream)->json_str; \
    } while (0)

#define SKIP_SPACES(s)          while (isspace(*(s))) s++
#define EXPECT_CHAR(s, ch)      SKIP_SPACES(s); MPIR_Assert(*s == (ch)); s++
#define SKIP_OVER_CHAR(s, ch)   while (*s && *s != (ch)) s++; MPIR_Assert(*s == (ch)); s++

static inline bool json_next(struct json_stream *json_stream, char **key)
{
    const char *s;
    JSON_STREAM_GET_S(json_stream, s);

    SKIP_SPACES(s);

    if (*s == '}') {
        JSON_STREAM_UPDATE_POS(json_stream, s);
        return false;
    } else {
        if (s[-1] != '{') {
            EXPECT_CHAR(s, ',');
        }

        EXPECT_CHAR(s, '"');
        const char *s_key = s;
        SKIP_OVER_CHAR(s, '"');
        *key = MPL_strdup_no_spaces(s_key, (s - s_key - 1));
        EXPECT_CHAR(s, ':');

        JSON_STREAM_UPDATE_POS(json_stream, s);
        return true;
    }
}

#define JSON_FOREACH(json_stream, key) \
    while (json_next(json_stream, &(key)))

#define JSON_FOREACH_START(json_stream) \
    do { \
        const char *s; \
        JSON_STREAM_GET_S(json_stream, s); \
        EXPECT_CHAR(s, '{'); \
        JSON_STREAM_UPDATE_POS(json_stream, s); \
    } while (0)

#define JSON_FOREACH_WRAP(json_stream) \
    do { \
        if ((json_stream)->json_str[(json_stream)->pos] == '}') { \
            (json_stream)->pos++; \
        } \
    } while (0)

static inline void json_skip_object(struct json_stream *json_stream)
{
    char *key;
    JSON_FOREACH_START(json_stream);
    JSON_FOREACH(json_stream, key) {
        MPL_free(key);
        json_skip_object(json_stream);
    }
    JSON_FOREACH_WRAP(json_stream);
}

#endif /* CSEL_JSON_H_INCLUDED */

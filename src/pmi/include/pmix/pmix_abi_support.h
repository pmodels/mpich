/*
 * Copyright (c) 2013-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2016-2019 Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * Copyright (c) 2016-2019 Mellanox Technologies, Inc.
 *                         All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2020      Cisco Systems, Inc.  All rights reserved
 * Copyright (c) 2021-2022 Nanook Consulting  All rights reserved.
 * Copyright (c) 2016-2022 IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/*
 * These PMIx ABI Support function are _not_ defined in the PMIx standard.
 * They exist here to provide a complete implementation for the macros
 * defined in pmix_macros.h
 * They are all prefixed with 'pmixabi_' to avoid the standard namespace
 * prefix of 'pmix_'.
 *
 * The PMIx ABI Support functions defined in this file _do not_ rely on
 * the macros defined in pmix_macros.h. So are included at the top
 * of that file.
 */
#ifndef PMIX_ABI_SUPPORT_H
#define PMIX_ABI_SUPPORT_H

/* define some "hooks" external libraries can use to
 * intercept memory allocation/release operations */
static inline void* pmixabi_malloc(size_t n)
{
    return malloc(n);
}

static inline void pmixabi_free(void *m)
{
    free(m);
}

static inline void* pmixabi_calloc(size_t n, size_t m)
{
    return calloc(n, m);
}

static inline
void pmixabi_argv_free(char **argv)
{
    if (NULL != (argv)) {
        char **p;
        for (p = (argv); NULL != *p; ++p) {
            free(*p);
        }
        free(argv);
    }
}

static inline
int pmixabi_argv_count(char **argv)
{
    char **p;
    int i;

    if (NULL == argv) {
        return 0;
    }

    for (i = 0, p = argv; *p; i++, p++) {
        continue;
    }

    return i;
}

static inline
pmix_status_t pmixabi_argv_append_nosize(char ***argv, const char *arg)
{
    int argc;

    /* Create new argv. */
    if (NULL == *argv) {
        *argv = (char **) malloc(2 * sizeof(char *));
        if (NULL == *argv) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        argc = 0;
        (*argv)[0] = NULL;
        (*argv)[1] = NULL;
    }
    /* Extend existing argv. */
    else {
        /* count how many entries currently exist */
        argc = pmixabi_argv_count(*argv);

        *argv = (char **) realloc(*argv, (argc + 2) * sizeof(char *));
        if (NULL == *argv) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
    }

    /* Set the newest element to point to a copy of the arg string */
    (*argv)[argc] = strdup(arg);
    if (NULL == (*argv)[argc]) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    argc = argc + 1;
    (*argv)[argc] = NULL;

    return PMIX_SUCCESS;
}

static inline
pmix_status_t pmixabi_argv_prepend_nosize(char ***argv, const char *arg)
{
    int argc;
    int i;

    /* Create new argv. */
    if (NULL == *argv) {
        *argv = (char **) malloc(2 * sizeof(char *));
        if (NULL == *argv) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        (*argv)[0] = strdup(arg);
        (*argv)[1] = NULL;
    } else {
        /* count how many entries currently exist */
        argc = pmixabi_argv_count(*argv);

        *argv = (char **) realloc(*argv, (argc + 2) * sizeof(char *));
        if (NULL == *argv) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        (*argv)[argc + 1] = NULL;

        /* shift all existing elements down 1 */
        for (i = argc; 0 < i; i--) {
            (*argv)[i] = (*argv)[i - 1];
        }
        (*argv)[0] = strdup(arg);
    }

    return PMIX_SUCCESS;
}

static inline
pmix_status_t pmixabi_argv_append_unique_nosize(char ***argv, const char *arg)
{
    int i;

    /* if the provided array is NULL, then the arg cannot be present,
     * so just go ahead and append
     */
    if (NULL == *argv) {
        return pmixabi_argv_append_nosize(argv, arg);
    }

    /* see if this arg is already present in the array */
    for (i = 0; NULL != (*argv)[i]; i++) {
        if (0 == strcmp(arg, (*argv)[i])) {
            /* already exists */
            return PMIX_SUCCESS;
        }
    }

    /* we get here if the arg is not in the array - so add it */
    return pmixabi_argv_append_nosize(argv, arg);
}

static inline
char **pmixabi_argv_split(const char *src_string, int delimiter)
{
    char **argv = NULL;
    char *p, *ptr;
    char *argtemp;
    int rc;

    argtemp = strdup(src_string);
    p = argtemp;
    while ('\0' != *p) {
        /* zero length argument, skip */
        if (NULL == (ptr = strchr(p, delimiter))) {
            // append the remainder and we are done
            rc = pmixabi_argv_append_nosize(&argv, p);
            if (PMIX_SUCCESS != rc) {
                pmixabi_argv_free(argv);
                free(argtemp);
                return NULL;
            }
            free(argtemp);
            return argv;
        }
        *ptr = '\0';
        ++ptr;
        rc = pmixabi_argv_append_nosize(&argv, p);
        if (PMIX_SUCCESS != rc) {
            pmixabi_argv_free(argv);
            free(argtemp);
            return NULL;
        }
        p = ptr;
    }
    free(argtemp);

    /* All done */
    return argv;
}

static inline
char *pmixabi_argv_join(char **argv, int delimiter)
{
    char **p;
    char *pp;
    char *str;
    size_t str_len = 0;
    size_t i;

    /* Bozo case */
    if (NULL == argv || NULL == argv[0]) {
        return strdup("");
    }

    /* Find the total string length in argv including delimiters.  The
     last delimiter is replaced by the NULL character. */
    for (p = argv; *p; ++p) {
        str_len += strlen(*p) + 1;
    }

    /* Allocate the string. */
    if (NULL == (str = (char *) malloc(str_len))) {
        return NULL;
    }

    /* Loop filling in the string. */
    str[--str_len] = '\0';
    p = argv;
    pp = *p;

    for (i = 0; i < str_len; ++i) {
        if ('\0' == *pp) {
            /* End of a string, fill in a delimiter and go to the next
             string. */
            str[i] = (char) delimiter;
            ++p;
            pp = *p;
        } else {
            str[i] = *pp++;
        }
    }

    /* All done */
    return str;
}

static inline
char **pmixabi_argv_copy(char **argv)
{
    char **dupv = NULL;

    if (NULL == argv) {
        return NULL;
    }

    /* create an "empty" list, so that we return something valid if we
     were passed a valid list with no contained elements */
    dupv = (char **) malloc(sizeof(char *));
    dupv[0] = NULL;

    while (NULL != *argv) {
        if (PMIX_SUCCESS != pmixabi_argv_append_nosize(&dupv, *argv)) {
            pmixabi_argv_free(dupv);
            return NULL;
        }

        ++argv;
    }

    /* All done */
    return dupv;
}

static inline
pmix_status_t pmixabi_setenv(const char *name,
                          const char *value,
                          bool overwrite,
                          char ***env)
{
    int i;
    char newvalue[2048], compare[2048];
    size_t len;
    bool valid;

    /* Check the bozo case */
    if (NULL == env) {
        return PMIX_ERR_BAD_PARAM;
    }

    if (NULL != value) {
        valid = false;
        for (i = 0; i < 100000; i++) {
            if ('\0' == value[i]) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            return PMIX_ERR_BAD_PARAM;
        }
    }

    /* If this is the "environ" array, use setenv */
    if (*env == environ) {
        if (NULL == value) {
            /* this is actually an unsetenv request */
            unsetenv(name);
        } else {
            setenv(name, value, overwrite);
        }
        return PMIX_SUCCESS;
    }

    /* Make the new value */
    if (NULL == value) {
        snprintf(newvalue, 2048, "%s=", name);
    } else {
        snprintf(newvalue, 2048, "%s=%s", name, value);
    }

    if (NULL == *env) {
        pmixabi_argv_append_nosize(env, newvalue);
        return PMIX_SUCCESS;
    }

    /* Make something easy to compare to */
    snprintf(compare, 2048, "%s=", name);
    len = strlen(compare);

    /* Look for a duplicate that's already set in the env */
    for (i = 0; (*env)[i] != NULL; ++i) {
        if (0 == strncmp((*env)[i], compare, len)) {
            if (overwrite) {
                free((*env)[i]);
                (*env)[i] = strdup(newvalue);
                return PMIX_SUCCESS;
            } else {
                return PMIX_ERR_BAD_PARAM;
            }
        }
    }

    /* If we found no match, append this value */
    pmixabi_argv_append_nosize(env, newvalue);

    /* All done */
    return PMIX_SUCCESS;
}

static inline
void pmixabi_strncpy(char *dest,
                  const char *src,
                  size_t len)
{
    size_t i;

    /* use an algorithm that also protects against
     * non-NULL-terminated src strings */
    for (i=0; i < len; ++i, ++src, ++dest) {
        *dest = *src;
        if ('\0' == *src) {
            break;
        }
    }
    *dest = '\0';
}

static inline
size_t pmixabi_keylen(const char *src)
{
    size_t i, maxlen;

    if (NULL == src) {
        return 0;
    }
    maxlen = PMIX_MAX_KEYLEN + 1;
    /* use an algorithm that also protects against
     * non-NULL-terminated src strings */
    for (i=0; i < maxlen; ++i, ++src) {
        if ('\0' == *src) {
            break;
        }
    }
    return i;
}

static inline
size_t pmixabi_nslen(const char *src)
{
    size_t i, maxlen;

    if (NULL == src) {
        return 0;
    }
    maxlen = PMIX_MAX_NSLEN + 1;
    /* use an algorithm that also protects against
     * non-NULL-terminated src strings */
    for (i=0; i < maxlen; ++i, ++src) {
        if ('\0' == *src) {
            break;
        }
    }
    return i;
}

/*
 * Declared here, but defined in pmix_abi_support_bottom.h
 */
static inline
void pmixabi_darray_destruct(pmix_data_array_t *m);
static inline
void pmixabi_value_destruct(pmix_value_t * m);

/* PMIX_ABI_SUPPORT_H */
#endif

/* -*- Mode: C; c-basic-offset:4 ; -*-
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "smpd.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#undef FCNAME
#define FCNAME "first_token"
static const char * first_token(const char *str)
{
    smpd_enter_fn(FCNAME);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return NULL;
    }
    while (isspace(*str))
	str++;
    if (*str == '\0')
    {
	smpd_exit_fn(FCNAME);
	return NULL;
    }
    smpd_exit_fn(FCNAME);
    return str;
}

#undef FCNAME
#define FCNAME "next_token"
static const char * next_token(const char *str)
{
    const char *result;

    smpd_enter_fn(FCNAME);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return NULL;
    }
    str = first_token(str);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return NULL;
    }
    if (*str == SMPD_QUOTE_CHAR)
    {
	/* move over string */
	str++; /* move over the first quote */
	if (*str == '\0')
	{
	    smpd_exit_fn(FCNAME);
	    return NULL;
	}
	while (*str != SMPD_QUOTE_CHAR)
	{
	    /* move until the last quote, ignoring escaped quotes */
	    if (*str == SMPD_ESCAPE_CHAR)
	    {
		str++;
		if (*str == SMPD_QUOTE_CHAR)
		    str++;
	    }
	    else
	    {
		str++;
	    }
	    if (*str == '\0')
	    {
		smpd_exit_fn(FCNAME);
		return NULL;
	    }
	}
	str++; /* move over the last quote */
    }
    else
    {
	if (*str == SMPD_DELIM_CHAR)
	{
	    /* move over the DELIM token */
	    str++;
	}
	else
	{
	    /* move over literal */
	    while (!isspace(*str) && *str != SMPD_DELIM_CHAR && *str != '\0')
		str++;
	}
    }
    result = first_token(str);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "compare_token"
static int compare_token(const char *token, const char *str)
{
    smpd_enter_fn(FCNAME);

    if (token == NULL || str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return -1;
    }

    if (*token == SMPD_QUOTE_CHAR)
    {
	/* compare quoted strings */
	token++; /* move over the first quote */
	/* compare characters until reaching the end of the string or the end quote character */
	for (;;)
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		{
		    /* move over the escape character if the next character is a quote character */
		    token++;
		}
		if (*token != *str)
		    break;
	    }
	    else
	    {
		if (*token != *str || *token == SMPD_QUOTE_CHAR)
		    break;
	    }
	    if (*str == '\0')
		break;
	    token++;
	    str++;
	}
	if (*str == '\0' && *token == SMPD_QUOTE_CHAR)
	{
	    smpd_exit_fn(FCNAME);
	    return 0;
	}
	if (*token == SMPD_QUOTE_CHAR)
	{
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
	if (*str < *token)
	{
	    smpd_exit_fn(FCNAME);
	    return -1;
	}
	smpd_exit_fn(FCNAME);
	return 1;
    }

    /* compare DELIM token */
    if (*token == SMPD_DELIM_CHAR)
    {
	if (*str == SMPD_DELIM_CHAR)
	{
	    str++;
	    if (*str == '\0')
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
	if (*token < *str)
	{
	    smpd_exit_fn(FCNAME);
	    return -1;
	}
	smpd_exit_fn(FCNAME);
	return 1;
    }

    /* compare literals */
    while (*token == *str && *str != '\0' && *token != SMPD_DELIM_CHAR && !isspace(*token))
    {
	token++;
	str++;
    }
    if ( (*str == '\0') && (*token == SMPD_DELIM_CHAR || isspace(*token) || *token == '\0') )
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }
    if (*token == SMPD_DELIM_CHAR || isspace(*token) || *token < *str)
    {
	smpd_exit_fn(FCNAME);
	return -1;
    }
    smpd_exit_fn(FCNAME);
    return 1;
}

#undef FCNAME
#define FCNAME "token_copy"
static void token_copy(const char *token, char *str, int maxlen)
{
    smpd_enter_fn(FCNAME);
    /* check parameters */
    if (token == NULL || str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return;
    }

    /* check special buffer lengths */
    if (maxlen < 1)
    {
	smpd_exit_fn(FCNAME);
	return;
    }
    if (maxlen == 1)
    {
	*str = '\0';
	smpd_exit_fn(FCNAME);
	return;
    }

    /* cosy up to the token */
    token = first_token(token);
    if (token == NULL)
    {
	smpd_exit_fn(FCNAME);
	return;
    }

    if (*token == SMPD_DELIM_CHAR)
    {
	/* copy the special deliminator token */
	str[0] = SMPD_DELIM_CHAR;
	str[1] = '\0';
	smpd_exit_fn(FCNAME);
	return;
    }

    if (*token == SMPD_QUOTE_CHAR)
    {
	/* quoted copy */
	token++; /* move over the first quote */
	do
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		    token++;
		*str = *token;
	    }
	    else
	    {
		if (*token == SMPD_QUOTE_CHAR)
		{
		    *str = '\0';
		    smpd_exit_fn(FCNAME);
		    return;
		}
		*str = *token;
	    }
	    str++;
	    token++;
	    maxlen--;
	} while (maxlen);
	/* we've run out of destination characters so back up and null terminate the string */
	str--;
	*str = '\0';
	smpd_exit_fn(FCNAME);
	return;
    }

    /* literal copy */
    while (*token != SMPD_DELIM_CHAR && !isspace(*token) && *token != '\0' && maxlen)
    {
	*str = *token;
	str++;
	token++;
	maxlen--;
    }
    if (maxlen)
	*str = '\0';
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "token_hide"
static void token_hide(char *token)
{
    smpd_enter_fn(FCNAME);
    /* check parameters */
    if (token == NULL)
    {
	smpd_exit_fn(FCNAME);
	return;
    }

    /* cosy up to the token */
    token = (char*)first_token(token);
    if (token == NULL)
    {
	smpd_exit_fn(FCNAME);
	return;
    }

    if (*token == SMPD_DELIM_CHAR)
    {
	*token = SMPD_HIDE_CHAR;
	smpd_exit_fn(FCNAME);
	return;
    }

    /* quoted */
    if (*token == SMPD_QUOTE_CHAR)
    {
	*token = SMPD_HIDE_CHAR;
	token++; /* move over the first quote */
	while (*token != '\0')
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		{
		    *token = SMPD_HIDE_CHAR;
		    token++;
		}
		*token = SMPD_HIDE_CHAR;
	    }
	    else
	    {
		if (*token == SMPD_QUOTE_CHAR)
		{
		    *token = SMPD_HIDE_CHAR;
		    smpd_exit_fn(FCNAME);
		    return;
		}
		*token = SMPD_HIDE_CHAR;
	    }
	    token++;
	}
	smpd_exit_fn(FCNAME);
	return;
    }

    /* literal */
    while (*token != SMPD_DELIM_CHAR && !isspace(*token) && *token != '\0')
    {
	*token = SMPD_HIDE_CHAR;
	token++;
    }
    smpd_exit_fn(FCNAME);
}

#undef FCNAME
#define FCNAME "smpd_get_string_arg"
int smpd_get_string_arg(const char *str, const char *flag, char *val, int maxlen)
{
    smpd_enter_fn(FCNAME);
    if (maxlen < 1)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FALSE;
    }

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, SMPD_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_FALSE;
		}
		token_copy(str, val, maxlen);
		smpd_exit_fn(FCNAME);
		return SMPD_TRUE;
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_hide_string_arg"
int smpd_hide_string_arg(char *str, const char *flag)
{
    smpd_enter_fn(FCNAME);
    /* line up with the first token */
    str = (char*)first_token(str);
    if (str == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = (char*)next_token(str);
	    if (compare_token(str, SMPD_DELIM_STR) == 0)
	    {
		str = (char*)next_token(str);
		if (str == NULL)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_SUCCESS;
		}
		token_hide(str);
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	}
	else
	{
	    str = (char*)next_token(str);
	}
    } while (str);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_int_arg"
int smpd_get_int_arg(const char *str, const char *flag, int *val_ptr)
{
    char int_str[12];

    smpd_enter_fn(FCNAME);
    if (smpd_get_string_arg(str, flag, int_str, 12))
    {
	*val_ptr = atoi(int_str);
	smpd_exit_fn(FCNAME);
	return SMPD_TRUE;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "quoted_printf"
static int quoted_printf(char *str, int maxlen, const char *val)
{
    int count = 0;
    smpd_enter_fn(FCNAME);
    if (maxlen < 1)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }
    *str = SMPD_QUOTE_CHAR;
    str++;
    maxlen--;
    count++;
    while (maxlen)
    {
	if (*val == '\0')
	    break;
	if (*val == SMPD_QUOTE_CHAR)
	{
	    *str = SMPD_ESCAPE_CHAR;
	    str++;
	    maxlen--;
	    count++;
	    if (maxlen == 0)
	    {
		smpd_exit_fn(FCNAME);
		return count;
	    }
	}
	*str = *val;
	str++;
	maxlen--;
	count++;
	val++;
    }
    if (maxlen)
    {
	*str = SMPD_QUOTE_CHAR;
	str++;
	maxlen--;
	count++;
	if (maxlen == 0)
	{
	    smpd_exit_fn(FCNAME);
	    return count;
	}
	*str = '\0';
    }
    smpd_exit_fn(FCNAME);
    return count;
}

#undef FCNAME
#define FCNAME "smpd_add_string"
int smpd_add_string(char *str, int maxlen, const char *val)
{
    int num_chars;

    smpd_enter_fn(FCNAME);
    if (strstr(val, " ") || val[0] == SMPD_QUOTE_CHAR)
    {
	num_chars = quoted_printf(str, maxlen, val);
	if (num_chars < maxlen)
	{
	    str[num_chars] = SMPD_SEPAR_CHAR;
	    str[num_chars+1] = '\0';
	}
	num_chars++;
    }
    else
    {
	num_chars = snprintf(str, maxlen, "%s ", val);
    }
    smpd_exit_fn(FCNAME);
    return num_chars;
}

#undef FCNAME
#define FCNAME "smpd_get_string"
const char * smpd_get_string(const char *str, char *val, int maxlen, int *num_chars)
{
    smpd_enter_fn(FCNAME);
    if (maxlen < 1)
    {
	*num_chars = 0;
	smpd_exit_fn(FCNAME);
	return NULL;
    }

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
    {
	*num_chars = 0;
	smpd_exit_fn(FCNAME);
	return NULL;
    }

    /* copy the token */
    token_copy(str, val, maxlen);
    *num_chars = (int)strlen(val);

    /* move to the next token */
    str = next_token(str);

    smpd_exit_fn(FCNAME);
    return str;
}

#undef FCNAME
#define FCNAME "smpd_add_string_arg"
int smpd_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *flag, const char *val)
{
    int num_chars;

    smpd_enter_fn(FCNAME);
    if (*maxlen_ptr < 1)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* add the flag */
    if (strstr(flag, " ") || strstr(flag, SMPD_DELIM_STR) || flag[0] == SMPD_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, flag);
    }
    else
    {
	num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s", flag);
    }
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 1)
    {
	(*str_ptr)[num_chars-1] = '\0';
	smpd_dbg_printf("partial argument added to string: '%s'\n", *str_ptr);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = SMPD_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
    if (strstr(val, " ") || strstr(val, SMPD_DELIM_STR) || val[0] == SMPD_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, val);
    }
    else
    {
	num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s", val);
    }
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 2)
    {
	*str_ptr = *str_ptr - 1;
	**str_ptr = '\0';
	smpd_dbg_printf("partial argument added to string: '%s'\n", *str_ptr);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    
    /* add the trailing space */
    **str_ptr = SMPD_SEPAR_CHAR;
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_add_int_arg"
int smpd_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *flag, int val)
{
    int result;
    char val_str[12];
    smpd_enter_fn(FCNAME);
    sprintf(val_str, "%d", val);
    result = smpd_add_string_arg(str_ptr, maxlen_ptr, flag, val_str);
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_get_opt"
int smpd_get_opt(int *argc, char ***argv, char * flag)
{
    int i,j;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    for (j=i; j<*argc; j++)
	    {
		(*argv)[j] = (*argv)[j+1];
	    }
	    *argc -= 1;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_opt_int"
int smpd_get_opt_int(int *argc, char ***argv, char * flag, int *n)
{
    int i,j;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == *argc)
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    *n = atoi((*argv)[i+1]);
	    for (j=i; j<*argc-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_opt_long"
int smpd_get_opt_long(int *argc, char ***argv, char * flag, long *n)
{
    int i;

    smpd_enter_fn(FCNAME);
    if (smpd_get_opt_int(argc, argv, flag, &i))
    {
	*n = (long)i;
	smpd_exit_fn(FCNAME);
	return 1;
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_opt_double"
int smpd_get_opt_double(int *argc, char ***argv, char * flag, double *d)
{
    int i,j;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == *argc)
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    *d = atof((*argv)[i+1]);
	    for (j=i; j<*argc-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_opt_string"
int smpd_get_opt_string(int *argc, char ***argv, char * flag, char *str, int len)
{
    int i,j;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == (*argc))
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    if ((*argv)[i+1][0] == '-')
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    strncpy(str, (*argv)[i+1], len);
	    str[len-1] = '\0';
	    for (j=i; j<(*argc)-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_opt_two_strings"
int smpd_get_opt_two_strings(int *argc, char ***argv, char * flag, char *str1, int len1, char *str2, int len2)
{
    int i, j;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == (*argc))
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    if ((*argv)[i+1][0] == '-')
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    if (i+2 == (*argc))
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    if ((*argv)[i+2][0] == '-')
	    {
		smpd_exit_fn(FCNAME);
		return 0;
	    }
	    strncpy(str1, (*argv)[i+1], len1);
	    str1[len1-1] = '\0';
	    strncpy(str2, (*argv)[i+2], len2);
	    str2[len2-1] = '\0';
	    for (j=i; j<(*argc)-2; j++)
	    {
		(*argv)[j] = (*argv)[j+3];
	    }
	    *argc -= 3;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

#undef FCNAME
#define FCNAME "smpd_get_win_opt_string"
int smpd_get_win_opt_string(int *argc, char ***argv, char * flag, char *str, int len)
{
    int i,j;
    char *iter;

    smpd_enter_fn(FCNAME);
    if (flag == NULL)
    {
	smpd_exit_fn(FCNAME);
	return 0;
    }

    for (i=0; i<*argc; i++)
    {
	if (strncmp((*argv)[i], flag, strlen(flag)) == 0)
	{
	    if (strlen(flag) == strlen((*argv)[i]))
	    {
		/* option specified with no value: /flag */
		continue;
	    }
	    iter = &(*argv)[i][strlen(flag)];
	    if ((*iter != ':') && (*iter != '=')) /* /flag=val or /flag:val */
	    {
		/* partial option match: /flag=val == /flagger=val */
		continue;
	    }
	    iter++;
	    strncpy(str, iter, len);
	    str[len-1] = '\0';
	    for (j=i; j<(*argc); j++)
	    {
		(*argv)[j] = (*argv)[j+1];
	    }
	    *argc -= 1;
	    smpd_exit_fn(FCNAME);
	    return 1;
	}
    }
    smpd_exit_fn(FCNAME);
    return 0;
}

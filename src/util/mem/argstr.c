/* -*- Mode: C; c-basic-offset:4 ; -*-
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpichconf.h"
#include "mpimem.h"
#include "mpidbg.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
/* ctype is needed for isspace and isascii (isspace is only defined for 
   values on which isascii returns true). */
#include <ctype.h>

#define MPIU_STR_TRUNCATED MPIU_STR_NOMEM

static int encode_buffer(char *dest, int dest_length, const char *src, 
			 int src_length, int *num_encoded)
{
    int num_used;
    int n = 0;
    char ch;
    if (src_length == 0)
    {
	if (dest_length > 2)
	{
	    *dest = MPIU_STR_QUOTE_CHAR;
	    dest++;
	    *dest = MPIU_STR_QUOTE_CHAR;
	    dest++;
	    *dest = '\0';
	    *num_encoded = 0;
	    return MPIU_STR_SUCCESS;
	}
	else
	{
	    return MPIU_STR_TRUNCATED;
	}
    }
    while (src_length && dest_length)
    {
	ch = *src;
	num_used = MPIU_Snprintf(dest, dest_length, "%02X", 
				 (unsigned char)*src);
	if (num_used < 0)
	{
	    *num_encoded = n;
	    return MPIU_STR_TRUNCATED;
	}
	/*MPIU_DBG_MSG_FMT(OTHER,VERBOSE,(MPIU_DBG_FDEST," %c = %c%c", 
	  ch, dest[0], dest[1]));*/
	dest += num_used;
	dest_length -= num_used;
	src++;
	n++;
	src_length--;
    }
    *num_encoded = n;
    return src_length ? MPIU_STR_TRUNCATED : MPIU_STR_SUCCESS;
}

static int decode_buffer(const char *str, char *dest, int length, 
			 int *num_decoded)
{
    char hex[3];
    int value;
    int n = 0;

    if (str == NULL || dest == NULL || num_decoded == NULL)
	return MPIU_STR_FAIL;
    if (length < 1)
    {
	*num_decoded = 0;
	if (*str == '\0')
	    return MPIU_STR_SUCCESS;
	return MPIU_STR_TRUNCATED;
    }
    if (*str == MPIU_STR_QUOTE_CHAR)
	str++;
    hex[2] = '\0';
    while (*str != '\0' &&
	   *str != MPIU_STR_SEPAR_CHAR &&
	   *str != MPIU_STR_QUOTE_CHAR &&
	   length)
    {
	hex[0] = *str;
	str++;
	hex[1] = *str;
	str++;
	sscanf(hex, "%X", &value);
	*dest = (char)value;
	/*MPIU_DBG_MSG_FMT(OTHER,VERBOSE,(MPIU_DBG_FDEST," %s = %c\n", 
	  hex, *dest));*/
	dest++;
	n++;
	length--;
    }
    *num_decoded = n;
    if (length == 0)
    {
	if (*str != '\0' &&
	    *str != MPIU_STR_SEPAR_CHAR &&
	    *str != MPIU_STR_QUOTE_CHAR)
	    return MPIU_STR_TRUNCATED;
    }
    return MPIU_STR_SUCCESS;
}

static const char * first_token(const char *str)
{
    if (str == NULL)
	return NULL;
    /* isspace is defined only if isascii is true */
    while (/*isascii(*str) && isspace(*str)*/ *str == MPIU_STR_SEPAR_CHAR)
	str++;
    if (*str == '\0')
	return NULL;
    return str;
}

static const char * next_token(const char *str)
{
    if (str == NULL)
	return NULL;
    str = first_token(str);
    if (str == NULL)
	return NULL;
    if (*str == MPIU_STR_QUOTE_CHAR)
    {
	/* move over string */
	str++; /* move over the first quote */
	if (*str == '\0')
	    return NULL;
	while (*str != MPIU_STR_QUOTE_CHAR)
	{
	    /* move until the last quote, ignoring escaped quotes */
	    if (*str == MPIU_STR_ESCAPE_CHAR)
	    {
		str++;
		if (*str == MPIU_STR_QUOTE_CHAR)
		    str++;
	    }
	    else
	    {
		str++;
	    }
	    if (*str == '\0')
		return NULL;
	}
	str++; /* move over the last quote */
    }
    else
    {
	if (*str == MPIU_STR_DELIM_CHAR)
	{
	    /* move over the DELIM token */
	    str++;
	}
	else
	{
	    /* move over literal */
	    while (/*(isascii(*str) &&
		    !isspace(*str)) &&*/
		    *str != MPIU_STR_SEPAR_CHAR &&
		   *str != MPIU_STR_DELIM_CHAR &&
		   *str != '\0')
		str++;
	}
    }
    return first_token(str);
}

static int compare_token(const char *token, const char *str)
{
    if (token == NULL || str == NULL)
	return -1;

    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	/* compare quoted strings */
	token++; /* move over the first quote */
	/* compare characters until reaching the end of the string or the 
	   end quote character */
	for(;;)
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		{
		    /* move over the escape character if the next character 
		       is a quote character */
		    token++;
		}
		if (*token != *str)
		    break;
	    }
	    else
	    {
		if (*token != *str || *token == MPIU_STR_QUOTE_CHAR)
		    break;
	    }
	    if (*str == '\0')
		break;
	    token++;
	    str++;
	}
	if (*str == '\0' && *token == MPIU_STR_QUOTE_CHAR)
	    return 0;
	if (*token == MPIU_STR_QUOTE_CHAR)
	    return 1;
	if (*str < *token)
	    return -1;
	return 1;
    }

    /* compare DELIM token */
    if (*token == MPIU_STR_DELIM_CHAR)
    {
	if (*str == MPIU_STR_DELIM_CHAR)
	{
	    str++;
	    if (*str == '\0')
		return 0;
	    return 1;
	}
	if (*token < *str)
	    return -1;
	return 1;
    }

    /* compare literals */
    while (*token == *str &&
	   *str != '\0' &&
	   *token != MPIU_STR_DELIM_CHAR && 
	   (*token != MPIU_STR_SEPAR_CHAR) )
    {
	token++;
	str++;
    }
    if ( (*str == '\0') &&
	 (*token == MPIU_STR_DELIM_CHAR ||
	  (*token == MPIU_STR_SEPAR_CHAR) || *token == '\0') )
	return 0;
    if (*token == MPIU_STR_DELIM_CHAR || 
	(*token == MPIU_STR_SEPAR_CHAR) || *token < *str)
	return -1;
    return 1;
}


static int token_copy(const char *token, char *str, int maxlen)
{
    /* check parameters */
    if (token == NULL || str == NULL)
	return MPIU_STR_FAIL;

    /* check special buffer lengths */
    if (maxlen < 1)
	return MPIU_STR_FAIL;
    if (maxlen == 1)
    {
	*str = '\0';
	return (str[0] == '\0') ? MPIU_STR_SUCCESS : MPIU_STR_TRUNCATED;
    }

    /* cosy up to the token */
    token = first_token(token);
    if (token == NULL)
    {
	*str = '\0';
	return MPIU_STR_SUCCESS;
    }

    if (*token == MPIU_STR_DELIM_CHAR)
    {
	/* copy the special deliminator token */
	str[0] = MPIU_STR_DELIM_CHAR;
	str[1] = '\0';
	return MPIU_STR_SUCCESS;
    }

    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	/* quoted copy */
	token++; /* move over the first quote */
	do
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		    token++;
		*str = *token;
	    }
	    else
	    {
		if (*token == MPIU_STR_QUOTE_CHAR)
		{
		    *str = '\0';
		    return MPIU_STR_SUCCESS;
		}
		*str = *token;
	    }
	    str++;
	    token++;
	    maxlen--;
	} while (maxlen);
	/* we've run out of destination characters so back up and null 
	   terminate the string */
	str--;
	*str = '\0';
	return MPIU_STR_TRUNCATED;
    }

    /* literal copy */
    while (*token != MPIU_STR_DELIM_CHAR && 
	   (*token != MPIU_STR_SEPAR_CHAR) && *token != '\0' && maxlen)
    {
	*str = *token;
	str++;
	token++;
	maxlen--;
    }
    if (maxlen)
    {
	*str = '\0';
	return MPIU_STR_SUCCESS;
    }
    str--;
    *str = '\0';
    return MPIU_STR_TRUNCATED;
}

static void token_hide(char *token)
{
    /* check parameters */
    if (token == NULL)
	return;

    /* cosy up to the token */
    token = (char*)first_token(token);
    if (token == NULL)
	return;

    if (*token == MPIU_STR_DELIM_CHAR)
    {
	*token = MPIU_STR_HIDE_CHAR;
	return;
    }

    /* quoted */
    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	*token = MPIU_STR_HIDE_CHAR;
	token++; /* move over the first quote */
	while (*token != '\0')
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		{
		    *token = MPIU_STR_HIDE_CHAR;
		    token++;
		}
		*token = MPIU_STR_HIDE_CHAR;
	    }
	    else
	    {
		if (*token == MPIU_STR_QUOTE_CHAR)
		{
		    *token = MPIU_STR_HIDE_CHAR;
		    return;
		}
		*token = MPIU_STR_HIDE_CHAR;
	    }
	    token++;
	}
	return;
    }

    /* literal */
    while (*token != MPIU_STR_DELIM_CHAR && 
	   (*token != MPIU_STR_SEPAR_CHAR) && *token != '\0')
    {
	*token = MPIU_STR_HIDE_CHAR;
	token++;
    }
}

/*@ MPIU_Str_get_string_arg - Extract an option from a string with a 
  maximum length
  
    Input Parameters:
+   str - Source string
.   key - key
-   maxlen - Maximum total length of 'val'

    Output Parameter:
.   val - output string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string

  Module:
  Utility
  @*/
int MPIU_Str_get_string_arg(const char *str, const char *flag, char *val, 
			    int maxlen)
{
    if (maxlen < 1)
	return MPIU_STR_FAIL;

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
	return MPIU_STR_FAIL;

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		    return MPIU_STR_FAIL;
		return token_copy(str, val, maxlen);
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    return MPIU_STR_FAIL;
}

/*@ MPIU_Str_get_binary_arg - Extract an option from a string with a maximum 
  length
  
    Input Parameters:
+   str - Source string
.   key - key
-   maxlen - Maximum total length of 'buffer'

    Output Parameter:
+   buffer - output buffer
-   out_length - output length

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string and decodes 
    the value
    back to binary data.  The data must have been encoded with 
    MPIU_Str_add_binary_arg.

  Module:
  Utility
  @*/
int MPIU_Str_get_binary_arg(const char *str, const char *flag, char *buffer, 
			    int maxlen, int *out_length)
{
    if (maxlen < 1)
	return MPIU_STR_FAIL;

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
	return MPIU_STR_FAIL;

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		    return MPIU_STR_FAIL;
		return decode_buffer(str, buffer, maxlen, out_length);
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    return MPIU_STR_FAIL;
}

/*@ MPIU_Str_hide_string_arg - Over-write the value of an string option 
  with * characters
  
    Input Parameters:
+   str - input string
-   key - key

    Output Parameter:
.   str - The string data is modified if the key is found in the string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_FAIL

    Notes:
    This routine covers an option in a string converting "key = value" to 
    "key = *****"

  Module:
  Utility
  @*/
MPIU_BOOL MPIU_Str_hide_string_arg(char *str, const char *flag)
{
    /* line up with the first token */
    str = (char*)first_token(str);
    if (str == NULL)
	return MPIU_TRUE;

    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = (char*)next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = (char*)next_token(str);
		if (str == NULL)
		    return MPIU_TRUE;
		token_hide(str);
		return MPIU_TRUE;
	    }
	}
	else
	{
	    str = (char*)next_token(str);
	}
    } while (str);
    return MPIU_FALSE;
}

/*@ MPIU_Str_get_int_arg - Extract an option from a string
  
    Input Parameters:
+   str - Source string
-   key - key

    Output Parameter:
.   val_ptr - pointer to the output integer

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine searches for a "key = value" entry in a string and decodes the value
    back to an int.

  Module:
  Utility
  @*/
int MPIU_Str_get_int_arg(const char *str, const char *flag, int *val_ptr)
{
    int result;
    char int_str[12];

    result = MPIU_Str_get_string_arg(str, flag, int_str, 12);
    if (result == MPIU_STR_SUCCESS)
    {
	*val_ptr = atoi(int_str);
	return MPIU_STR_SUCCESS;
    }
    return result;
}

/* quoted_printf does not NULL terminate the string if maxlen is reached */
static int quoted_printf(char *str, int maxlen, const char *val)
{
    int count = 0;
    if (maxlen < 1)
	return 0;
    *str = MPIU_STR_QUOTE_CHAR;
    str++;
    maxlen--;
    count++;
    while (maxlen)
    {
	if (*val == '\0')
	    break;
	if (*val == MPIU_STR_QUOTE_CHAR)
	{
	    *str = MPIU_STR_ESCAPE_CHAR;
	    str++;
	    maxlen--;
	    count++;
	    if (maxlen == 0)
		return count;
	}
	*str = *val;
	str++;
	maxlen--;
	count++;
	val++;
    }
    if (maxlen)
    {
	*str = MPIU_STR_QUOTE_CHAR;
	str++;
	maxlen--;
	count++;
	if (maxlen == 0)
	    return count;
	*str = '\0';
    }
    return count;
}

/*@ MPIU_Str_add_string - Add a string to a string
  
    Input Parameters:
+   str_ptr - pointer to the destination string
.   maxlen_ptr - pointer to the maximum length of '*str_ptr'
-   val - string to add

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in 
    the string
-   maxlen_ptr - maxlen is decremented by the amount str_ptr is incremented

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds a string to a string in such a way that 
    MPIU_Str_get_string can
    retreive the same string back.  It takes into account spaces and quote 
    characters.
    The string pointer is updated to the start of the next string in the 
    string and maxlen is updated accordingly.

  Module:
  Utility
  @*/
int MPIU_Str_add_string(char **str_ptr, int *maxlen_ptr, const char *val)
{
    int num_chars;
    char *str;
    int maxlen;

    str = *str_ptr;
    maxlen = *maxlen_ptr;

    if (strchr(val, MPIU_STR_SEPAR_CHAR) ||
	strchr(val, MPIU_STR_QUOTE_CHAR) ||
	strchr(val, MPIU_STR_DELIM_CHAR))
    {
	num_chars = quoted_printf(str, maxlen, val);
	if (num_chars == maxlen)
	{
	    /* truncation, cleanup string */
	    *str = '\0';
	    return -1;
	}
	if (num_chars < maxlen - 1)
	{
	    str[num_chars] = MPIU_STR_SEPAR_CHAR;
	    str[num_chars+1] = '\0';
	    num_chars++;
	}
	else
	{
	    str[num_chars] = '\0';
	}
    }
    else
    {
	if (*val == '\0')
	{
	    num_chars = MPIU_Snprintf(str, maxlen, 
		      MPIU_STR_QUOTE_STR MPIU_STR_QUOTE_STR/*"\"\""*/);
	}
	else
	{
	    num_chars = MPIU_Snprintf(str, maxlen, "%s%c", val, 
				      MPIU_STR_SEPAR_CHAR);
	}
	if (num_chars == maxlen)
	{
	    *str = '\0';
	    return -1;
	}
    }
    *str_ptr += num_chars;
    *maxlen_ptr -= num_chars;
    return 0;
}

/*@ MPIU_Str_get_string - Get the next string from a string
  
    Input Parameters:
+   str_ptr - pointer to the destination string
-   maxlen_ptr - pointer to the maximum length of '*str_ptr'

    Output Parameter:
+   str_ptr - location of the next string
-   val - location to store the string

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Return Value:
    The return value is 0 for success, -1 for insufficient buffer space, and 
    1 for failure.

    Notes:
    This routine gets a string that was previously added by 
    MPIU_Str_add_string.
    It takes into account spaces and quote characters. The string pointer is 
    updated to the start of the next string in the string.

  Module:
  Utility
  @*/
int MPIU_Str_get_string(char **str_ptr, char *val, int maxlen)
{
    int result;
    char *str;

    if (str_ptr == NULL)
    {
	return -2;
    }

    str = *str_ptr;
    
    if (maxlen < 1)
    {
	return 0;
    }

    /* line up with the first token */
    str = (char*)first_token(str);
    if (str == NULL)
    {
	return 0;
    }

    /* copy the token */
    result = token_copy(str, val, maxlen);
    if (result == MPIU_STR_SUCCESS)
    {
	str = (char*)next_token(str);
	*str_ptr = str;
	return 0;
    }
    else if (result == MPIU_STR_TRUNCATED)
    {
	return -1;
    }

    /* failure */
    return -2;
}

/*@ MPIU_Str_add_string_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
-   val - input string

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in
    the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds a string option to a string in the form "key = value".

  Module:
  Utility
  @*/
int MPIU_Str_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *flag, 
			    const char *val)
{
    int num_chars;
    char **orig_str_ptr;
    int orig_maxlen;

    if (maxlen_ptr == NULL)
	return MPIU_STR_FAIL;

    orig_maxlen = *maxlen_ptr;
    orig_str_ptr = str_ptr;

    if (*maxlen_ptr < 1)
	return MPIU_STR_FAIL;

    /* add the flag */
/*     printf("strstr flag\n"); */
    if (strstr(flag, MPIU_STR_SEPAR_STR) || strstr(flag, MPIU_STR_DELIM_STR) ||
	flag[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, flag);
    }
    else
    {
	num_chars = MPIU_Snprintf(*str_ptr, *maxlen_ptr, "%s", flag);
    }
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 1)
    {
	MPIU_DBG_MSG_S(OTHER,VERBOSE,
                  "partial argument added to string: '%s'", *str_ptr);
	**str_ptr = '\0';
	/*(*str_ptr)[num_chars-1] = '\0';*/
	return MPIU_STR_NOMEM;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = MPIU_STR_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
/*     printf("strstr val\n"); */
    if (strstr(val, MPIU_STR_SEPAR_STR) || strstr(val, MPIU_STR_DELIM_STR) ||
	val[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, val);
    }
    else
    {
	if (*val == '\0')
	{
	    num_chars = MPIU_Snprintf(*str_ptr, *maxlen_ptr, 
			      MPIU_STR_QUOTE_STR MPIU_STR_QUOTE_STR/*"\"\""*/);
	}
	else
	{
	    num_chars = MPIU_Snprintf(*str_ptr, *maxlen_ptr, "%s", val);
	}
    }
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 2)
    {
	MPIU_DBG_MSG_S(OTHER,VERBOSE,
		       "partial argument added to string: '%s'", *str_ptr);
	**orig_str_ptr = '\0';
	/*
	*str_ptr = *str_ptr - 1;
	**str_ptr = '\0';
	*/
	return MPIU_STR_NOMEM;
    }
    
    /* add the trailing space */
    **str_ptr = MPIU_STR_SEPAR_CHAR;
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    return MPIU_STR_SUCCESS;
}

/*@ MPIU_Str_add_int_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
-   val - input integer

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in 
    the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine adds an integer option to a string in the form "key = value".

  Module:
  Utility
  @*/
int MPIU_Str_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *flag, 
			 int val)
{
    char val_str[12];
    MPIU_Snprintf(val_str, 12, "%d", val);
    return MPIU_Str_add_string_arg(str_ptr, maxlen_ptr, flag, val_str);
}

/*@ MPIU_Str_add_binary_arg - Add an option to a string with a maximum length
  
    Input Parameters:
+   str_ptr - Pointer to the destination string
.   maxlen_ptr - Pointer to the maximum total length of '*str_ptr'
.   key - key
.   val - input data
-   length - length of the input data

    Output Parameter:
+   str_ptr - The string pointer is updated to the next available location in 
    the string
-   maxlen_ptr - maxlen is reduced by the number of characters written

    Return value:
    MPIU_STR_SUCCESS, MPIU_STR_NOMEM, MPIU_STR_FAIL

    Notes:
    This routine encodes binary data into a string option in the form 
    "key = encoded_value".

  Module:
  Utility
  @*/
int MPIU_Str_add_binary_arg(char **str_ptr, int *maxlen_ptr, const char *flag,
			    const char *buffer, int length)
{
    int result;
    int num_chars;
    char **orig_str_ptr;
    int orig_maxlen;

    if (maxlen_ptr == NULL)
	return MPIU_STR_FAIL;

    orig_maxlen = *maxlen_ptr;
    orig_str_ptr = str_ptr;

    if (*maxlen_ptr < 1)
	return MPIU_STR_FAIL;

    /* add the flag */
    if (strstr(flag, MPIU_STR_SEPAR_STR) || strstr(flag, MPIU_STR_DELIM_STR) ||
	flag[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, flag);
    }
    else
    {
	num_chars = MPIU_Snprintf(*str_ptr, *maxlen_ptr, "%s", flag);
    }
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 1)
    {
	MPIU_DBG_MSG_S(OTHER,VERBOSE,
		       "partial argument added to string: '%s'", *str_ptr);
	**str_ptr = '\0';
	/*(*str_ptr)[num_chars-1] = '\0';*/
	return MPIU_STR_NOMEM;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = MPIU_STR_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
    result = encode_buffer(*str_ptr, *maxlen_ptr, buffer, length, &num_chars);
    if (result != MPIU_STR_SUCCESS)
    {
	**orig_str_ptr = '\0';
	return result;
    }
    num_chars = num_chars * 2; /* the encoding function turns one source 
				  character into two destination characters */
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 2)
    {
	MPIU_DBG_MSG_S(OTHER,VERBOSE,
		       "partial argument added to string: '%s'", *str_ptr);
	/*
	*str_ptr = *str_ptr - 1;
	**str_ptr = '\0';
	*/
	**orig_str_ptr = '\0';
	return MPIU_STR_NOMEM;
    }
    
    /* add the trailing space */
    **str_ptr = MPIU_STR_SEPAR_CHAR;
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    return MPIU_STR_SUCCESS;
}

/*
int test_argstrings()
{
    char buffer[1024];
    char buffer2[1024];
    char key[100];
    char val[1024];
    int maxlen, maxlen2;
    char *iter, *iter2;
    int ibuf[3] = { 1, 2, 3 };
    int age;

    iter = buffer;
    maxlen = 1024;
    MPIU_Str_add_string_arg(&iter, &maxlen, "name", "David Ashton");
    MPIU_Str_add_string_arg(&iter, &maxlen, "a b", "c d");
    MPIU_Str_add_binary_arg(&iter, &maxlen, "array", ibuf, 12);
    MPIU_Str_add_string_arg(&iter, &maxlen, "=", "equals");
    MPIU_Str_add_string_arg(&iter, &maxlen, "\"help=", "=\"");
    MPIU_Str_add_int_arg(&iter, &maxlen, "age", 123);

    printf("encoded buffer: <%s>\n", buffer);

    MPIU_Str_get_string_arg(buffer, "name", val, 1024);
    printf("name = '%s'\n", val);
    MPIU_Str_get_string_arg(buffer, "a b", val, 1024);
    printf("a b = '%s'\n", val);
    MPIU_Str_get_string_arg(buffer, "=", val, 1024);
    printf("= = '%s'\n", val);
    MPIU_Str_get_string_arg(buffer, "\"help=", val, 1024);
    printf("\"help= = '%s'\n", val);
    MPIU_Str_get_int_arg(buffer, "age", &age);
    printf("age = %d\n", age);
    MPIU_Str_get_binary_arg(buffer, "array", ibuf, 12);
    printf("ibuf[0] = %d\n", ibuf[0]);
    printf("ibuf[1] = %d\n", ibuf[1]);
    printf("ibuf[2] = %d\n", ibuf[2]);

    iter = buffer2;
    maxlen = 1024;
    MPIU_Str_add_string_arg(&iter, &maxlen, "big arg", buffer);
    MPIU_Str_add_string(&iter, &maxlen, "Once isn't enough for me");
    MPIU_Str_add_int_arg(&iter, &maxlen, "size", 100);
    printf("doubly encoded buffer: <%s>\n", buffer2);

    MPIU_Str_get_string_arg(buffer2, "big arg", buffer, 1024);
    printf("big arg = '%s'\n", buffer);
    MPIU_Str_get_string_arg(buffer, "name", val, 1024);
    printf("name = '%s'\n", val);
    MPIU_Str_get_string_arg(buffer, "a b", val, 1024);
    printf("a b = '%s'\n", val);
    MPIU_Str_get_string_arg(buffer, "=", val, 1024);
    printf("= = '%s'\n", val);
    MPIU_Str_get_int_arg(buffer, "age", &age);
    printf("age = %d\n", age);
    MPIU_Str_get_binary_arg(buffer, "array", ibuf, 12);
    printf("ibuf[0] = %d\n", ibuf[0]);
    printf("ibuf[1] = %d\n", ibuf[1]);
    printf("ibuf[2] = %d\n", ibuf[2]);

    MPIU_Str_get_int_arg(buffer2, "size", &age);
    printf("size = %d\n", age);
    return 0;
}
*/

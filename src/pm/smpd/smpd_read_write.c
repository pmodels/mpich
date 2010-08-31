/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

static char hexchar(int x)
{
    switch (x)
    {
    case 0: return '0';
    case 1: return '1';
    case 2: return '2';
    case 3: return '3';
    case 4: return '4';
    case 5: return '5';
    case 6: return '6';
    case 7: return '7';
    case 8: return '8';
    case 9: return '9';
    case 10: return 'A';
    case 11: return 'B';
    case 12: return 'C';
    case 13: return 'D';
    case 14: return 'E';
    case 15: return 'F';
    }
    return '0';
}

static char charhex(char c)
{
    switch (c)
    {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    }
    return 0;
}

static void char_to_hex(char ch, char *hex)
{
    *hex = hexchar((ch>>4) & 0xF);
    hex++;
    *hex = hexchar(ch & 0xF);
}

static char hex_to_char(const char *hex)
{
    unsigned char ch;
    ch = charhex(*hex) << 4;
    hex++;
    ch = ch | charhex(*hex);
    return (char)ch;
}

int smpd_encode_buffer(char *dest, int dest_length, const char *src, int src_length, int *num_encoded)
{
    int n = 0;
    while (src_length && (dest_length > 2))
    {
	char_to_hex(*src, dest);
	src++;
	dest += 2;
	src_length--;
	dest_length -= 2;
	n++;
    }
    *dest = '\0';
    *num_encoded = n;
    return SMPD_SUCCESS;
}

int smpd_decode_buffer(const char *str, char *dest, int length, int *num_decoded)
{
    int n = 0;

    if (length < 1)
    {
	return SMPD_FAIL;
    }

    while (*str != '\0' && length)
    {
	*dest = hex_to_char(str);
	str += 2;
	dest++;
	length--;
	n++;
    }
    *num_decoded = n;
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_read"
int smpd_read(SMPDU_Sock_t sock, void *buf, SMPDU_Sock_size_t len)
{
    int result;
    SMPDU_Size_t num_read;

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("reading %d bytes from sock %d\n", len, SMPDU_Sock_get_sock_id(sock));

    while (len)
    {
	/* aggressively write */
	result = SMPDU_Sock_read(sock, buf, len, &num_read);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to read %d bytes,\nsock error: %s\n", len, get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_read == len)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}

	if (num_read == 0)
	{
#ifdef HAVE_WINDOWS_H
	    Sleep(1);
#elif defined(HAVE_USLEEP)
	    usleep(1000);
#endif
	}
	else
	{
	    buf = (char*)buf + num_read;
	    len -= num_read;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_write"
int smpd_write(SMPDU_Sock_t sock, void *buf, SMPDU_Sock_size_t len)
{
    int result;
    SMPDU_Size_t num_written;

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("writing %d bytes to sock %d\n", len, SMPDU_Sock_get_sock_id(sock));

    while (len)
    {
	/* aggressively write */
	result = SMPDU_Sock_write(sock, buf, len, &num_written);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write %d bytes,\nsock error: %s\n",
			    len, get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_written == len)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}

	if (num_written == 0)
	{
	    smpd_dbg_printf("0 bytes written.\n");
#ifdef HAVE_WINDOWS_H
	    Sleep(1);
#elif defined(HAVE_USLEEP)
	    usleep(1000);
#endif
	}
	else
	{
	    smpd_dbg_printf("wrote %d bytes\n", num_written);
	    buf = (char*)buf + num_written;
	    len -= num_written;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_write_string"
int smpd_write_string(SMPDU_Sock_t sock, char *str)
{
    int result;
    SMPDU_Size_t len, num_written;

    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("writing string on sock %d: \"%s\"\n", SMPDU_Sock_get_sock_id(sock), str);

    len = (SMPDU_Size_t)strlen(str)+1;

    while (len)
    {
	/* aggressively write string */
	result = SMPDU_Sock_write(sock, str, len, &num_written);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write string of length %d,\nsock error: %s\n", len, get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_written == len)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}

	/* post a write for whatever is left of the string */
	str += num_written;
	len -= num_written;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

static int read_string(SMPDU_Sock_t sock, char *str, int maxlen)
{
    char ch;
    int result;
    SMPDU_Size_t num_bytes;
    int total = 0;

    if (maxlen < 1)
	return 0;
    result = SMPDU_Sock_read(sock, &ch, 1, &num_bytes);
    while (result == SMPD_SUCCESS)
    {
	if (num_bytes == 0)
	    return total;
	total++;
	*str = ch;
	str++;
	if (ch == '\0' || total >= maxlen)
	    return total;
	result = SMPDU_Sock_read(sock, &ch, 1, &num_bytes);
    }
    smpd_err_printf("Unable to read a string,\nsock error: %s\n", get_sock_error_string(result));
    return -1;
}

static int chew_up_string(SMPDU_Sock_t sock)
{
    char ch;
    int result;

    result = smpd_read_string(sock, &ch, 1);
    while (result == SMPD_SUCCESS)
    {
	if (ch == '\0')
	    return SMPD_SUCCESS;
	result = smpd_read_string(sock, &ch, 1);
    }
    smpd_err_printf("Unable to read a string,\nsock error: %s\n", get_sock_error_string(result));
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_read_string"
int smpd_read_string(SMPDU_Sock_t sock, char *str, int maxlen)
{
    int num_bytes;
    char *str_orig;

    smpd_enter_fn(FCNAME);

    str_orig = str;

    if (maxlen == 0)
    {
	smpd_dbg_printf("zero length read string request on sock %d\n", SMPDU_Sock_get_sock_id(sock));
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    for (;;)
    {
	num_bytes = read_string(sock, str, maxlen);
	if (num_bytes == -1)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (num_bytes > 0 && str[num_bytes-1] == '\0')
	{
	    smpd_dbg_printf("received string on sock %d: \"%s\"\n", SMPDU_Sock_get_sock_id(sock), str_orig);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	if (num_bytes == maxlen)
	{
	    /* received truncated string */
	    str[num_bytes-1] = '\0';
	    chew_up_string(sock);
	    smpd_dbg_printf("received truncated string on sock %d: \"%s\"\n", SMPDU_Sock_get_sock_id(sock), str_orig);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	str += num_bytes;
	maxlen -= num_bytes;
    }
    /*
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
    */
}

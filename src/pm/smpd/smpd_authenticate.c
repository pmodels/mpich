/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_getpid()
{
#ifdef HAVE_WINDOWS_H
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

#undef FCNAME
#define FCNAME "smpd_gen_authentication_strings"
int smpd_gen_authentication_strings(char *phrase, char *append, char *crypted)
{
    int stamp;
    char hash[SMPD_PASSPHRASE_MAX_LENGTH];
    char phrase_internal[SMPD_PASSPHRASE_MAX_LENGTH+1];

    smpd_enter_fn(FCNAME);

    stamp = rand();

    snprintf(phrase_internal, SMPD_PASSPHRASE_MAX_LENGTH, "%s%s %d", phrase, SMPD_VERSION, stamp);
    snprintf(append, SMPD_AUTHENTICATION_STR_LEN, "%s %d", SMPD_VERSION, stamp);

    smpd_hash(phrase_internal, (int)strlen(phrase_internal), hash, SMPD_PASSPHRASE_MAX_LENGTH);
    if (strlen(hash) > SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("internal crypted string too long: %d > %d\n", strlen(hash), SMPD_PASSPHRASE_MAX_LENGTH);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(crypted, hash);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
static const char * crypt_error(int error)
{
    static char unknown[100];
    switch (error)
    {
    case ERROR_INVALID_HANDLE:
	return "One of the parameters specifies an invalid handle.";
    case ERROR_INVALID_PARAMETER:
	return "One of the parameters contains an invalid value. This is most often an invalid pointer.";
    case NTE_BAD_ALGID:
	return "The hKey session key specifies an algorithm that this CSP does not support.";
    case NTE_BAD_DATA:
	return "The data to be decrypted is invalid. For example, when a block cipher is used and the Final flag is FALSE, the value specified by pdwDataLen must be a multiple of the block size. This error can also be returned when the padding is found to be invalid.";
    case NTE_BAD_FLAGS:
	return "The dwFlags parameter is nonzero.";
    case NTE_BAD_HASH:
	return "The hHash parameter contains an invalid handle.";
    case NTE_BAD_KEY:
	return "The hKey parameter does not contain a valid handle to a key.";
    case NTE_BAD_LEN:
	return "The size of the output buffer is too small to hold the generated plaintext.";
    case NTE_BAD_UID:
	return "The CSP context that was specified when the key was created cannot be found.";
    case NTE_DOUBLE_ENCRYPT:
	return "The application attempted to decrypt the same data twice.";
    case NTE_FAIL:
	return "Failure";
    }
    sprintf(unknown, "Unknown failure %d", error);
    return unknown;
}
#endif

#undef FCNAME
#define FCNAME "smpd_encrypt_data_plaintext"
static int smpd_encrypt_data_plaintext(char *input, int input_length, char *output, int output_length)
{
    smpd_enter_fn(FCNAME);
    if (output_length < input_length + 2)
    {
	smpd_err_printf("encryption output buffer too small: %d < %d.\n", output_length, input_length + 2);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    *output = SMPD_PLAINTEXT_PREFIX;
    output++;
    output_length--;
    memcpy(output, input, input_length);
    output[input_length] = '\0';
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_encrypt_data"
int smpd_encrypt_data(char *input, int input_length, char *output, int output_length)
{
#ifdef HAVE_WINDOWS_H
    HCRYPTPROV hCryptProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    CHAR szPassword[SMPD_MAX_PASSWORD_LENGTH] = "";
    DWORD dwLength;
    char *buffer = NULL;
    int length, result;
    int ret_val = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    if (smpd_process.plaintext)
    {
	ret_val = smpd_encrypt_data_plaintext(input, input_length, output, output_length);
	smpd_exit_fn(FCNAME);
	return ret_val;
    }

    /* Acquire a cryptographic provider context handle. */
    if (!CryptAcquireContext(&hCryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
	/* Error creating key container!*/
	smpd_err_printf("Error during CryptAcquireContext: %d\n", GetLastError());
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Create an empty hash object. */
    if (!CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash)) 
    {
	smpd_err_printf("Error during CryptCreateHash\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Hash the password string. */
    /*MPIU_Snprintf(szPassword, SMPD_MAX_PASSWORD_LENGTH, "%s %s", smpd_process.encrypt_prefix, smpd_process.passphrase);*/
    MPIU_Strncpy(szPassword, smpd_process.passphrase, SMPD_MAX_PASSWORD_LENGTH);
    /*smpd_dbg_printf("phrase to generate the encryption key: '%s'\n", szPassword);*/
    dwLength = (DWORD)strlen(szPassword);
    if (!CryptHashData(hHash, (BYTE *)szPassword, dwLength, 0)) 
    {
	smpd_err_printf("Error during CryptHashData\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Create a session key based on the hash of the smpd password. */
    if (!CryptDeriveKey(hCryptProv, CALG_RC2, hHash, CRYPT_EXPORTABLE, &hKey)) 
    {
	smpd_err_printf("Error during CryptDeriveKey\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Allocate a temporary buffer to work on */
    buffer = (char*)MPIU_Malloc(input_length * 2);
    if (buffer == NULL)
    {
	smpd_err_printf("MPIU_Malloc returned NULL\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }
    /* copy the input data to the temporary buffer and encrypt in-place */
    memcpy(buffer, input, input_length);
    dwLength = input_length;
    if (!CryptEncrypt(hKey, 0, TRUE, 0, (BYTE*)buffer, &dwLength, input_length*2))
    {
	smpd_err_printf("Error during CryptEncrypt: %s\n", crypt_error(GetLastError()));
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* encode the binary encrypted blob to the output text buffer */
    *output = SMPD_ENCRYPTED_PREFIX;
    output++;
    output_length--;
    result = smpd_encode_buffer(output, output_length, buffer, dwLength, &length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to encode the encrypted password\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }
    /*smpd_dbg_printf("encrypted and encoded password: %d bytes = '%s'\n", length, output);*/

fn_cleanup:
    if (buffer)
    {
	MPIU_Free(buffer);
    }

    /* Destroy the hash object. */
    if (hHash)
    {
	CryptDestroyHash(hHash);
    }

    /* Destroy the session key. */
    if (hKey)
    {
	CryptDestroyKey(hKey);
    }

    /* Release the provider handle. */
    if (hCryptProv)
    {
	CryptReleaseContext(hCryptProv, 0);
    }

    smpd_exit_fn(FCNAME);
    return ret_val;

#else
    int ret_val = SMPD_SUCCESS;
    smpd_enter_fn(FCNAME);
    ret_val = smpd_encrypt_data_plaintext(input, input_length, output, output_length);
    smpd_exit_fn(FCNAME);
    return ret_val;
#endif
}

#undef FCNAME
#define FCNAME "smpd_decrypt_data_plaintext"
static int smpd_decrypt_data_plaintext(char *input, int input_length, char *output, int *output_length)
{
    smpd_enter_fn(FCNAME);
    /* check the prefix character to make sure the data is plaintext */
    if (*input != SMPD_PLAINTEXT_PREFIX)
    {
	smpd_err_printf("decryption module not available, please try again with -plaintext.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* skip over the prefix character */
    input++;
    input_length--;
    if (*output_length < input_length)
    {
	smpd_err_printf("decryption output buffer too small.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    memcpy(output, input, input_length);
    *output_length = input_length;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_decrypt_data"
int smpd_decrypt_data(char *input, int input_length, char *output, int *output_length)
{
#ifdef HAVE_WINDOWS_H
    HCRYPTPROV hCryptProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    CHAR szPassword[SMPD_MAX_PASSWORD_LENGTH] = "";
    DWORD dwLength;
    char *buffer = NULL;
    int length, result;
    int ret_val = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);

    if (*input == SMPD_PLAINTEXT_PREFIX)
    {
	ret_val = smpd_decrypt_data_plaintext(input, input_length, output, output_length);
	smpd_exit_fn(FCNAME);
	return ret_val;
    }

    /* Acquire a cryptographic provider context handle. */
    if (!CryptAcquireContext(&hCryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
	/* Error creating key container!*/
	smpd_err_printf("Error during CryptAcquireContext: %d\n", GetLastError());
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Create an empty hash object. */
    if (!CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash)) 
    {
	smpd_err_printf("Error during CryptCreateHash\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Hash the password string. */
    /*MPIU_Snprintf(szPassword, SMPD_MAX_PASSWORD_LENGTH, "%s %s", smpd_process.encrypt_prefix, smpd_process.passphrase);*/
    MPIU_Strncpy(szPassword, smpd_process.passphrase, SMPD_MAX_PASSWORD_LENGTH);
    /*smpd_dbg_printf("phrase to generate the decryption key: '%s'\n", szPassword);*/
    dwLength = (DWORD)strlen(szPassword);
    if (!CryptHashData(hHash, (BYTE *)szPassword, dwLength, 0)) 
    {
	smpd_err_printf("Error during CryptHashData\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* Create a session key based on the hash of the smpd password. */
    if (!CryptDeriveKey(hCryptProv, CALG_RC2, hHash, CRYPT_EXPORTABLE, &hKey)) 
    {
	smpd_err_printf("Error during CryptDeriveKey\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* skip over the prefix character */
    input++;
    input_length--;

    /* Allocate a temporary buffer to work on */
    buffer = (char*)MPIU_Malloc(input_length);
    if (buffer == NULL)
    {
	smpd_err_printf("MPIU_Malloc returned NULL\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* decode the data back to it's binary encrypted format */
    /*smpd_dbg_printf("encoded encrypted password: '%s'\n", input);*/
    result = smpd_decode_buffer(input, buffer, input_length, &length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to decode the encrypted password\n");
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* decrypt the data blob back to plaintext */
    dwLength = length;
    if (!CryptDecrypt(hKey, 0, TRUE, 0, (BYTE*)buffer, &dwLength))
    {
	smpd_err_printf("Error during CryptDecrypt: %s\n", crypt_error(GetLastError()));
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }
    if (dwLength >= (DWORD)*output_length)
    {
	smpd_err_printf("decrypted buffer is larger than the output buffer supplied: %d >= %d\n", dwLength, *output_length);
	ret_val = SMPD_FAIL;
	goto fn_cleanup;
    }

    /* copy the plaintext to the output buffer */
    memcpy(output, buffer, dwLength);
    *output_length = dwLength;

fn_cleanup:
    if (buffer)
    {
	MPIU_Free(buffer);
    }

    /* Destroy the hash object. */
    if (hHash)
    {
	CryptDestroyHash(hHash);
    }

    /* Destroy the session key. */
    if (hKey)
    {
	CryptDestroyKey(hKey);
    }

    /* Release the provider handle. */
    if (hCryptProv)
    {
	CryptReleaseContext(hCryptProv, 0);
    }

    smpd_exit_fn(FCNAME);
    return ret_val;
#else
    int ret_val = SMPD_SUCCESS;
    smpd_enter_fn(FCNAME);
    ret_val = smpd_decrypt_data_plaintext(input, input_length, output, output_length);
    smpd_exit_fn(FCNAME);
    return ret_val;
#endif
}

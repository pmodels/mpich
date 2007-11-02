/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

#ifdef HAVE_WINDOWS_H

#include <windows.h>
#include <wincrypt.h>

#define HASH_LENGTH 16

#undef FCNAME
#define FCNAME "smpd_hash"
int smpd_hash(char *input, int input_length, char *output, int output_length)
{
    /*DWORD i;*/
    CRYPT_HASH_MESSAGE_PARA      hash_parameter;
    const BYTE*                  hash_buffer[1];
    DWORD                        hash_buffer_len[1];
    CRYPT_ALGORITHM_IDENTIFIER   algorithm;
    BYTE                         hash[HASH_LENGTH];
    DWORD                        hash_len = HASH_LENGTH;

    smpd_enter_fn(FCNAME);

    if (output_length < (HASH_LENGTH * 2 + 1))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    hash_buffer[0] = (const BYTE *)input;
    hash_buffer_len[0] = input_length;

    /* Initialize the CRYPT_ALGORITHM_IDENTIFIER data structure. */
    algorithm.pszObjId = szOID_RSA_MD5;
    algorithm.Parameters.cbData=0;

    /* Initialize the CRYPT_HASH_MESSAGE_PARA data structure. */
    hash_parameter.cbSize = sizeof(CRYPT_HASH_MESSAGE_PARA);
    hash_parameter.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    hash_parameter.hCryptProv = (HCRYPTPROV)NULL;
    hash_parameter.HashAlgorithm = algorithm;
    hash_parameter.pvHashAuxInfo = NULL;

    /* Calculate the size of the hashed message. */
    /*
    if (!CryptHashMessage(&hash_parameter, FALSE, 1, hash_buffer, hash_buffer_len, NULL, NULL, NULL, &hash_len))
    {
	return SMPD_FAIL;
    }
    if (hash_len > HASH_LENGTH)
    {
	return SMPD_FAIL;
    }
    */

    /* Hash the message. */
    if (CryptHashMessage(&hash_parameter, FALSE, 1, hash_buffer, hash_buffer_len, NULL, NULL, hash, &hash_len))
    {
	/*
	for (i=0; i<hash_len; i++)
	{
	    sprintf(output, "%02x", hash[i]);
	    output += 2;
	}
	*/
	sprintf(output, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	    hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
	    hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
    }
    else
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return 0;
}

#else

#ifdef HAVE_MD5_CALC
#include <md5.h>
#define MD5_DIGEST_LENGTH 16
#else
#include <openssl/md5.h>
#endif

#undef FCNAME
#define FCNAME "smpd_hash"
int smpd_hash(char *input, int input_length, char *output, int output_length)
{
    /*int i;*/
    unsigned char hash[MD5_DIGEST_LENGTH];

    smpd_enter_fn(FCNAME);

    if (output_length < (MD5_DIGEST_LENGTH * 2 + 1))
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

#ifdef HAVE_MD5_CALC
    md5_calc(hash, (unsigned char *)input, (unsigned int)input_length);
#else
    MD5(input, input_length, hash);
#endif

    /*
    for (i=0; i<MD5_DIGEST_LENGTH; i++)
    {
	sprintf(output, "%02x", hash[i]);
	output += 2;
    }
    */
    sprintf(output, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
	hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#endif

#ifdef SMPD_BUILD_MD5_TEST
/* Use MDTestSuite to test the smpd_hash code */
/* Digests a string and prints the result. */
static void MDString(char *string)
{
    unsigned char digest[100];
    unsigned int len = (unsigned int)strlen(string);

    smpd_hash(string, len, digest, 100);
    printf("MD5(\"%s\") = %s\n", string, digest);
}

/* Digests a reference suite of strings and prints the results. */
void MDTestSuite()
{
    printf ("MD5 test suite:\n");

    MDString("");
    MDString("a");
    MDString("abc");
    MDString("message digest");
    MDString("abcdefghijklmnopqrstuvwxyz");
    MDString("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    MDString("12345678901234567890123456789012345678901234567890123456789012345678901234567890");

    printf("correct output:\n");
    printf("MD5(\"\") = d41d8cd98f00b204e9800998ecf8427e\n");
    printf("MD5(\"a\") = 0cc175b9c0f1b6a831c399e269772661\n");
    printf("MD5(\"abc\") = 900150983cd24fb0d6963f7d28e17f72\n");
    printf("MD5(\"message digest\") = f96b697d7cb7938d525a2f31aaf161d0\n");
    printf("MD5(\"abcdefghijklmnopqrstuvwxyz\") = c3fcd3d76192e4007dfb496cca67e13b\n");
    printf("MD5(\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\") = d174ab98d277d9f5a5611c2c9f419d9f\n");
    printf("MD5(\"12345678901234567890123456789012345678901234567890123456789012345678901234567890\") = 57edf4a22be3c955ac49da2e2107b67a\n");
}

int main(int argc, char *argv[])
{
    MDTestSuite();
    return 0;
}
#endif

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

#ifdef HAVE_WINDOWS_H
/*
static void print_bytes(char *str, int length)
{
    int i;
    printf("%d bytes: '", length);
    for (i=0; i<length; i++)
    {
	if (str[i] > 20)
	{
	    printf("%c", str[i]);
	}
	else
	{
	    printf("(%d)", (int)str[i]);
	}
    }
    printf("'\n");
    fflush(stdout);
}
*/
void smpd_stdin_thread(SOCKET hWrite)
{
    DWORD num_read;
    char str[SMPD_MAX_CMD_LENGTH];
    int index;
    HANDLE h = INVALID_HANDLE_VALUE;
    DWORD result;
    char err_msg[256] = "";
    /*char bogus_char;*/

    smpd_dbg_printf("smpd_stdin_thread started.\n");
    /* acquire the launch process mutex to avoid grabbing a redirected input handle */
    result = WaitForSingleObject(smpd_process.hLaunchProcessMutex, INFINITE);
    if (result == WAIT_FAILED)
    {
	result = GetLastError();
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("smpd_stdin_thread:WaitForSingleObject(hLaunchProcessMutex) failed: Error %d, %s\n", result, err_msg);
	goto fn_fail;
    }
    h = GetStdHandle(STD_INPUT_HANDLE);
    if (!ReleaseMutex(smpd_process.hLaunchProcessMutex))
    {
	result = GetLastError();
	smpd_translate_win_error(result, err_msg, 256, NULL);
	smpd_err_printf("smpd_stdin_thread:ReleaseMutex(hLaunchProcessMutex) failed: Error %d, %s\n", result, err_msg);
	goto fn_fail;
    }
    if (h == NULL || h == INVALID_HANDLE_VALUE)
    {
	/* Don't print an error in case there is no stdin handle */
	smpd_dbg_printf("Unable to get the stdin handle.\n");
	goto fn_fail;
    }
    index = 0;
    for (;;)
    {
	/*smpd_dbg_printf("waiting for input from stdin\n");*/
	num_read = 0;
	str[index] = '\0';
	if (ReadFile(h, &str[index], 1, &num_read, NULL))
	{
	    if (num_read < 1)
	    {
		/* forward any buffered data */
		if (index > 0)
		{
		    if (send(hWrite, str, index, 0) == SOCKET_ERROR)
		    {
			result = WSAGetLastError();
			smpd_translate_win_error(result, err_msg, 256, NULL);
			smpd_err_printf("unable to forward stdin, send failed, error %d, %s\n", result, err_msg);
			goto fn_fail;
		    }
		}
		/* ReadFile failed, what do I do? */
		smpd_dbg_printf("ReadFile failed, closing stdin reader thread.\n");
		goto fn_fail;
	    }
	    /*printf("CHAR(%d)", (int)str[index]);fflush(stdout);*/
	    if (str[index] == '\n' || index == SMPD_MAX_CMD_LENGTH-1)
	    {
		num_read = index + 1;
		index = 0; /* reset the index back to the beginning of the input buffer */
		smpd_dbg_printf("forwarding stdin: %d bytes\n", num_read);
		/*print_bytes(str, num_read);*/
		if (send(hWrite, str, num_read, 0) == SOCKET_ERROR)
		{
		    result = WSAGetLastError();
		    smpd_translate_win_error(result, err_msg, 256, NULL);
		    smpd_err_printf("unable to forward stdin, send failed, error %d, %s\n", result, err_msg);
		    goto fn_fail;
		}
	    }
	    else
	    {
		index++;
	    }
	}
	else
	{
	    /* forward any buffered data */
	    if (index > 0)
	    {
		if (send(hWrite, str, index, 0) == SOCKET_ERROR)
		{
		    result = WSAGetLastError();
		    smpd_translate_win_error(result, err_msg, 256, NULL);
		    smpd_err_printf("unable to forward stdin, send failed, error %d, %s\n", result, err_msg);
		    goto fn_fail;
		}
	    }
	    /* ReadFile failed, what do I do? */
	    smpd_dbg_printf("ReadFile failed, closing stdin reader thread.\n");
	    goto fn_fail;
	}
    }
fn_exit:
    return;
fn_fail:
    /* graceful shutdown
    if (shutdown(hWrite, SD_SEND) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hWrite, WSAGetLastError());
    }
    recv(hWrite, &bogus_char, 1, 0);
    if (closesocket(hWrite) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hWrite, WSAGetLastError());
    }
    */
    if (shutdown(hWrite, SD_BOTH) == SOCKET_ERROR)
    {
	smpd_err_printf("shutdown failed, sock %d, error %d\n", hWrite, WSAGetLastError());
    }
    if (closesocket(hWrite) == SOCKET_ERROR)
    {
	smpd_err_printf("closesocket failed, sock %d, error %d\n", hWrite, WSAGetLastError());
    }
    goto fn_exit;
}
#else
#ifdef USE_PTHREAD_STDIN_REDIRECTION
void *smpd_pthread_stdin_thread(void *p)
{
    int read_fd, write_fd;
    char ch;
    size_t num_read;
    fd_set set;
    int n;
    /*struct timeval tv;*/

    write_fd = smpd_process.stdin_write;
    read_fd = fileno(stdin);

    for(;;)
    {
	FD_ZERO(&set);
	FD_SET(read_fd, &set);
	FD_SET(write_fd, &set);

	/*
	  tv.tv_sec = -1;
	  tv.tv_usec = -1;
	*/
	n = select(write_fd+1, &set, NULL, NULL, NULL/*&tv*/);
	if (n<0)
	{
	    if (errno = EINTR || errno == 0)
		continue;
	    goto fn_exit;
	}

	if (FD_ISSET(write_fd, &set))
	{
	    /* A byte is sent on the write_fd to inform the thread to exit */
	    read(write_fd, &ch, 1);
	    goto fn_exit;
	}
	if (FD_ISSET(read_fd, &set))
	{
	    num_read = read(read_fd, &ch, 1);
	    if (num_read > 0)
	    {
		if (write(write_fd, &ch, 1) != 1)
		{
		    smpd_dbg_printf("stdin redirection write failed, error %d\n", errno);
		    goto fn_exit;
		}
	    }
	    else if (num_read == 0)
	    {
		/* Read thread closed, should we exit? */
		goto fn_exit;
	    }
	    else if (num_read == -1)
	    {
		/* An error occurred while reading */
		if (errno != EINTR && errno != 0)
		{
		    smpd_err_printf("reading from stdin failed with error %d\n", errno);
		    goto fn_exit;
		}
	    }
	}
    }
fn_exit:
    close(write_fd);
    return NULL;
}

#undef FCNAME
#define FCNAME "smpd_cancel_stdin_thread"
int smpd_cancel_stdin_thread()
{
    char ch = 0;
    smpd_enter_fn(FCNAME);
    /*printf("cancelling stin redirection thread\n");*/
    if (smpd_process.stdin_thread == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    write(smpd_process.stdin_read, &ch, 1);
    pthread_join(smpd_process.stdin_thread, NULL);
    /*pthread_cancel(smpd_process.stdin_thread);*/
    smpd_process.stdin_thread = NULL;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#endif
#endif

#undef FCNAME
#define FCNAME "smpd_get_state_string"
const char * smpd_get_state_string(smpd_state_t state)
{
    static char unknown_str[100];
    const char *result;

    smpd_enter_fn(FCNAME);
    switch (state)
    {
    case SMPD_IDLE:                            result = "SMPD_IDLE";                            break;
    case SMPD_MPIEXEC_CONNECTING_TREE:         result = "SMPD_MPIEXEC_CONNECTING_TREE";         break;
    case SMPD_MPIEXEC_CONNECTING_SMPD:         result = "SMPD_MPIEXEC_CONNECTING_SMPD";         break;
    case SMPD_CONNECTING:                      result = "SMPD_CONNECTING";                      break;
    case SMPD_READING_CHALLENGE_STRING:        result = "SMPD_READING_CHALLENGE_STRING";        break;
    case SMPD_WRITING_CHALLENGE_RESPONSE:      result = "SMPD_WRITING_CHALLENGE_RESPONSE";      break;
    case SMPD_READING_CONNECT_RESULT:          result = "SMPD_READING_CONNECT_RESULT";          break;
    case SMPD_WRITING_CHALLENGE_STRING:        result = "SMPD_WRITING_CHALLENGE_STRING";        break;
    case SMPD_READING_CHALLENGE_RESPONSE:      result = "SMPD_READING_CHALLENGE_RESPONSE";      break;
    case SMPD_WRITING_CONNECT_RESULT:          result = "SMPD_WRITING_CONNECT_RESULT";          break;
    case SMPD_READING_STDIN:                   result = "SMPD_READING_STDIN";                   break;
    case SMPD_WRITING_DATA_TO_STDIN:           result = "SMPD_WRITING_DATA_TO_STDIN";           break;
    case SMPD_READING_STDOUT:                  result = "SMPD_READING_STDOUT";                  break;
    case SMPD_READING_STDERR:                  result = "SMPD_READING_STDERR";                  break;
    case SMPD_READING_CMD_HEADER:              result = "SMPD_READING_CMD_HEADER";              break;
    case SMPD_READING_CMD:                     result = "SMPD_READING_CMD";                     break;
    case SMPD_WRITING_CMD:                     result = "SMPD_WRITING_CMD";                     break;
    case SMPD_SMPD_LISTENING:                  result = "SMPD_SMPD_LISTENING";                  break;
    case SMPD_MGR_LISTENING:                   result = "SMPD_MGR_LISTENING";                   break;
    case SMPD_PMI_LISTENING: 	               result = "SMPD_PMI_LISTENING";                   break;
    case SMPD_SINGLETON_CLIENT_LISTENING:      result = "SMPD_SINGLETON_CLIENT_LISTENING";      break;
    case SMPD_SINGLETON_MPIEXEC_CONNECTING:    result = "SMPD_SINGLETON_MPIEXEC_CONNECTING";    break;
    case SMPD_SINGLETON_READING_PMI_INFO:      result = "SMPD_SINGLETON_READING_PMI_INFO";      break;
    case SMPD_SINGLETON_WRITING_PMI_INFO:      result = "SMPD_SINGLETON_WRITING_PMI_INFO";      break;
    case SMPD_SINGLETON_DONE:                  result = "SMPD_SINGLETON_DONE";                  break;
    case SMPD_PMI_SERVER_LISTENING:            result = "SMPD_PMI_SERVER_LISTENING";            break;
    case SMPD_READING_SESSION_REQUEST:         result = "SMPD_READING_SESSION_REQUEST";         break;
    case SMPD_WRITING_SMPD_SESSION_REQUEST:    result = "SMPD_WRITING_SMPD_SESSION_REQUEST";    break;
    case SMPD_WRITING_PROCESS_SESSION_REQUEST: result = "SMPD_WRITING_PROCESS_SESSION_REQUEST"; break;
    case SMPD_WRITING_PMI_SESSION_REQUEST:     result = "SMPD_WRITING_PMI_SESSION_REQUEST";     break;
    case SMPD_WRITING_PWD_REQUEST:             result = "SMPD_WRITING_PWD_REQUEST";             break;
    case SMPD_WRITING_NO_PWD_REQUEST:          result = "SMPD_WRITING_NO_PWD_REQUEST";          break;
    case SMPD_READING_PWD_REQUEST:             result = "SMPD_READING_PWD_REQUEST";             break;
    case SMPD_READING_SMPD_PASSWORD:           result = "SMPD_READING_SMPD_PASSWORD";           break;
    case SMPD_WRITING_CRED_REQUEST:            result = "SMPD_WRITING_CRED_REQUEST";            break;
    case SMPD_READING_CRED_ACK:                result = "SMPD_READING_CRED_ACK";                break;
    case SMPD_WRITING_CRED_ACK_YES:            result = "SMPD_WRITING_CRED_ACK_YES";            break;
    case SMPD_WRITING_CRED_ACK_NO:             result = "SMPD_WRITING_CRED_ACK_NO";             break;
    case SMPD_READING_ACCOUNT:                 result = "SMPD_READING_ACCOUNT";                 break;
    case SMPD_READING_PASSWORD:                result = "SMPD_READING_PASSWORD";                break;
    case SMPD_WRITING_ACCOUNT:                 result = "SMPD_WRITING_ACCOUNT";                 break;
    case SMPD_WRITING_PASSWORD:                result = "SMPD_WRITING_PASSWORD";                break;
    case SMPD_WRITING_NO_CRED_REQUEST:         result = "SMPD_WRITING_NO_CRED_REQUEST";         break;
    case SMPD_READING_CRED_REQUEST:            result = "SMPD_READING_CRED_REQUEST";            break;
    case SMPD_WRITING_RECONNECT_REQUEST:       result = "SMPD_WRITING_RECONNECT_REQUEST";       break;
    case SMPD_WRITING_NO_RECONNECT_REQUEST:    result = "SMPD_WRITING_NO_RECONNECT_REQUEST";    break;
    case SMPD_READING_RECONNECT_REQUEST:       result = "SMPD_READING_RECONNECT_REQUEST";       break;
    case SMPD_READING_SESSION_HEADER:          result = "SMPD_READING_SESSION_HEADER";          break;
    case SMPD_WRITING_SESSION_HEADER:          result = "SMPD_WRITING_SESSION_HEADER";          break;
    case SMPD_READING_SMPD_RESULT:             result = "SMPD_READING_SMPD_RESULT";             break;
    case SMPD_READING_PROCESS_RESULT:          result = "SMPD_READING_PROCESS_RESULT";          break;
    case SMPD_WRITING_SESSION_ACCEPT:          result = "SMPD_WRITING_SESSION_ACCEPT";          break;
    case SMPD_WRITING_SESSION_REJECT:          result = "SMPD_WRITING_SESSION_REJECT";          break;
    case SMPD_WRITING_PROCESS_SESSION_ACCEPT:  result = "SMPD_WRITING_PROCESS_SESSION_REJECT";  break;
    case SMPD_WRITING_PROCESS_SESSION_REJECT:  result = "SMPD_WRITING_PROCESS_SESSION_REJECT";  break;
    case SMPD_RECONNECTING:                    result = "SMPD_RECONNECTING";                    break;
    case SMPD_EXITING:                         result = "SMPD_EXITING";                         break;
    case SMPD_CLOSING:                         result = "SMPD_CLOSING";                         break;
    case SMPD_WRITING_SMPD_PASSWORD:           result = "SMPD_WRITING_SMPD_PASSWORD";           break;
    case SMPD_READING_SSPI_HEADER:             result = "SMPD_READING_SSPI_HEADER";             break;
    case SMPD_READING_SSPI_BUFFER:             result = "SMPD_READING_SSPI_BUFFER";             break;
    case SMPD_WRITING_SSPI_HEADER:             result = "SMPD_WRITING_SSPI_HEADER";             break;
    case SMPD_WRITING_SSPI_BUFFER:             result = "SMPD_WRITING_SSPI_BUFFER";             break;
    case SMPD_WRITING_DELEGATE_REQUEST:        result = "SMPD_WRITING_DELEGATE_REQUEST";        break;
    case SMPD_READING_DELEGATE_REQUEST_RESULT: result = "SMPD_READING_DELEGATE_REQUEST_RESULT"; break;
    case SMPD_WRITING_IMPERSONATE_RESULT:      result = "SMPD_WRITING_IMPERSONATE_RESULT";      break;
    case SMPD_WRITING_CRED_ACK_SSPI:           result = "SMPD_WRITING_CRED_ACK_SSPI";           break;
    case SMPD_READING_CLIENT_SSPI_HEADER:      result = "SMPD_READING_CLIENT_SSPI_HEADER";      break;
    case SMPD_READING_CLIENT_SSPI_BUFFER:      result = "SMPD_READING_CLIENT_SSPI_BUFFER";      break;
    case SMPD_WRITING_CLIENT_SSPI_HEADER:      result = "SMPD_WRITING_CLIENT_SSPI_HEADER";      break;
    case SMPD_WRITING_CLIENT_SSPI_BUFFER:      result = "SMPD_WRITING_CLIENT_SSPI_BUFFER";      break;
    case SMPD_READING_TIMEOUT:                 result = "SMPD_READING_TIMEOUT";                 break;
    case SMPD_READING_MPIEXEC_ABORT:           result = "SMPD_READING_MPIEXEC_ABORT";           break;
    case SMPD_RESTARTING:                      result = "SMPD_RESTARTING";                      break;
    case SMPD_DONE:                            result = "SMPD_DONE";                            break;
    case SMPD_CONNECTING_RPMI:                 result = "SMPD_CONNECTING_RPMI";                 break;
    case SMPD_CONNECTING_PMI:                  result = "SMPD_CONNECTING_PMI";                  break;
    case SMPD_WRITING_SSPI_REQUEST:            result = "SMPD_WRITING_SSPI_REQUEST";            break;
    case SMPD_READING_PMI_ID:                  result = "SMPD_READING_PMI_ID";                  break;
    case SMPD_WRITING_PMI_ID:                  result = "SMPD_WRITING_PMI_ID";                  break;
    case SMPD_WRITING_DELEGATE_REQUEST_RESULT: result = "SMPD_WRITING_DELEGATE_REQUEST_RESULT"; break;
    case SMPD_READING_IMPERSONATE_RESULT:      result = "SMPD_READING_IMPERSONATE_RESULT";      break;
    case SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY:   result = "SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY";   break;
    case SMPD_WRITING_SSPI_JOB_KEY:            result = "SMPD_WRITING_SSPI_JOB_KEY";            break;
    case SMPD_READING_SSPI_JOB_KEY:            result = "SMPD_READING_SSPI_JOB_KEY";            break;
    default:
	sprintf(unknown_str, "unknown state %d", state);
	result = unknown_str;
	break;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_verify_version"
SMPD_BOOL smpd_verify_version(const char *challenge)
{
    char version[100];
    char *space_char;

    /* The first part of the challenge string up to the first space is the smpd version */

    smpd_enter_fn(FCNAME);
    space_char = strchr(challenge, ' ');
    if (space_char != NULL && space_char - challenge < 100)
    {
	strncpy(version, challenge, space_char - challenge);
	version[space_char - challenge] = '\0';
	if (strcmp(version, SMPD_VERSION) == 0)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_TRUE;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_challenge_string"
int smpd_state_reading_challenge_string(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];

    smpd_enter_fn(FCNAME);

    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the challenge string, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read challenge string: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;

    /* check to see if we are simply trying to get the version */
    if (smpd_process.do_console == 1 && smpd_process.builtin_cmd == SMPD_CMD_VERSION)
    {
	strtok(context->pszChallengeResponse, " ");
	printf("%s\n", context->pszChallengeResponse);
	fflush(stdout);
	smpd_exit(0);
    }

    if (smpd_verify_version(context->pszChallengeResponse) == SMPD_TRUE)
    {
        smpd_dbg_printf("Verification of smpd version succeeded\n");
	strcpy(phrase, smpd_process.passphrase);
	/* crypt the passphrase + the challenge */
	if (strlen(phrase) + strlen(context->pszChallengeResponse) > SMPD_PASSPHRASE_MAX_LENGTH)
	{
	    smpd_err_printf("smpd_client_authenticate: unable to process passphrase - too long.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	strcat(phrase, context->pszChallengeResponse);

	/*smpd_dbg_printf("crypting: %s\n", phrase);*/
	smpd_hash(phrase, (int)strlen(phrase), context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN);
	/* save the response to be used as a prefix to the password when encrypting the password */
	MPIU_Strncpy(smpd_process.encrypt_prefix, context->pszChallengeResponse, SMPD_MAX_PASSWORD_LENGTH);
    }
    else
    {
        smpd_dbg_printf("Verification of smpd version failed...Sending version failure to PM\n");
	    strcpy(context->pszChallengeResponse, SMPD_VERSION_FAILURE);
    }

    /* write the response */
    /*smpd_dbg_printf("writing response: %s\n", pszStr);*/
    context->write_state = SMPD_WRITING_CHALLENGE_RESPONSE;
    result = SMPDU_Sock_post_write(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the encrypted response string,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_challenge_response"
int smpd_state_writing_challenge_response(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the challenge response, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote challenge response: '%s'\n", context->pszChallengeResponse);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CONNECT_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the connect response,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_connect_result"
int smpd_state_reading_connect_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the connect result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read connect result: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->pszChallengeResponse, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	char post_message[100];
	/* rejected connection, close */
	smpd_dbg_printf("connection rejected, server returned - %s\n", context->pszChallengeResponse);
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to close sock, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* connection failed, abort? */
	/* when does a forming context get assinged it's global place?  At creation?  At connection? */
	if (strcmp(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_VERSION_STR) == 0)
	{
	    /* Customize the abort message to state that the smpd version did not match */
	    strcpy(post_message, ", smpd version mismatch");
	}
	else
	{
	    post_message[0] = '\0';
	}
	if (smpd_process.left_context == context)
	    smpd_process.left_context = NULL;
	if (smpd_process.do_console && smpd_process.console_host[0] != '\0')
	    result = smpd_post_abort_command("unable to connect to %s%s", smpd_process.console_host, post_message);
	else if (context->connect_to && context->connect_to->host[0] != '\0')
	    result = smpd_post_abort_command("unable to connect to %s%s", context->connect_to->host, post_message);
	else
	{
	    if (context->host[0] != '\0')
	    {
		result = smpd_post_abort_command("unable to connect to %s%s", context->host, post_message);
	    }
	    else
	    {
		result = smpd_post_abort_command("connection to smpd rejected%s", post_message);
	    }
	}
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	context->write_state = SMPD_WRITING_PROCESS_SESSION_REQUEST;
	context->target = SMPD_TARGET_PROCESS;
	switch (context->state)
	{
	case SMPD_MPIEXEC_CONNECTING_TREE:
	case SMPD_CONNECTING:
	    strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    break;
	case SMPD_MPIEXEC_CONNECTING_SMPD:
	    if (smpd_process.use_process_session)
	    {
		strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    }
	    else
	    {
		context->target = SMPD_TARGET_SMPD;
		strcpy(context->session, SMPD_SMPD_SESSION_STR);
		context->write_state = SMPD_WRITING_SMPD_SESSION_REQUEST;
	    }
	    break;
	case SMPD_CONNECTING_RPMI:
	    context->target = SMPD_TARGET_PMI;
	    context->write_state = SMPD_WRITING_PMI_SESSION_REQUEST;
	    strcpy(context->session, SMPD_PMI_SESSION_STR);
	    break;
	default:
	    strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    break;
	}
	result = SMPDU_Sock_post_write(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session request '%s',\nsock error: %s\n",
		context->session, get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_challenge_string"
int smpd_state_writing_challenge_string(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the challenge string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	smpd_exit_fn(FCNAME);
	return result;
    }
    smpd_dbg_printf("wrote challenge string: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_READING_CHALLENGE_RESPONSE;
    context->write_state = SMPD_IDLE;
    result = SMPDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("posting a read of the challenge response string failed,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	smpd_exit_fn(FCNAME);
	return result;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_challenge_response"
int smpd_state_reading_challenge_response(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the challenge response, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read challenge response: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_CONNECT_RESULT;
    if (strcmp(context->pszChallengeResponse, SMPD_VERSION_FAILURE) == 0)
    {
	strcpy(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_VERSION_STR);
    }
    else if (strcmp(context->pszChallengeResponse, context->pszCrypt) == 0)
    {
	strcpy(context->pszChallengeResponse, SMPD_AUTHENTICATION_ACCEPTED_STR);
    }
    else
    {
	strcpy(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_STR);
    }
    result = SMPDU_Sock_post_write(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the connect result '%s',\nsock error: %s\n",
	    context->pszChallengeResponse, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_connect_result"
int smpd_state_writing_connect_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the connect result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote connect result: '%s'\n", context->pszChallengeResponse);
    context->write_state = SMPD_IDLE;
    if (strcmp(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_STR) == 0)
    {
	context->state = SMPD_CLOSING;
	smpd_dbg_printf("connection reject string written, closing sock.\n");
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_post_close failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    context->read_state = SMPD_READING_SESSION_REQUEST;
    result = SMPDU_Sock_post_read(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_stdin"
int smpd_state_reading_stdin(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    SMPDU_Size_t num_read;
    char buffer[SMPD_MAX_CMD_LENGTH];
    int num_encoded;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/*smpd_err_printf("unable to read from stdin, %s.\n", get_sock_error_string(event_ptr->error));*/
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read from stdin\n");
    if (context->type == SMPD_CONTEXT_MPIEXEC_STDIN)
    {
	smpd_dbg_printf("read from %s\n", smpd_get_context_str(context));

	/* one byte read, attempt to read up to the buffer size */
	result = SMPDU_Sock_read(context->sock, &context->read_cmd.cmd[1], SMPD_STDIN_PACKET_SIZE-1, &num_read);
	if (result != SMPD_SUCCESS)
	{
	    num_read = 0;
	    smpd_dbg_printf("SMPDU_Sock_read(%d) failed (%s), assuming %s is closed.\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
	}
	smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));
	smpd_encode_buffer(buffer, SMPD_MAX_CMD_LENGTH, context->read_cmd.cmd, num_read+1, &num_encoded);
	buffer[num_encoded*2] = '\0';
	/*smpd_dbg_printf("encoded %d characters: %d '%s'\n", num_encoded, strlen(buffer), buffer);*/

	/* create an stdin command */
	result = smpd_create_command("stdin", 0, 1 /* input always goes to node 1? */, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an stdin command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "data", buffer);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the data to the stdin command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* send the stdin command */
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the stdin command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else if (context->type == SMPD_CONTEXT_MPIEXEC_STDIN_RSH)
    {
	unsigned char *buf;
	SMPDU_Size_t total, num_written;
	smpd_dbg_printf("read from %s\n", smpd_get_context_str(context));

	/* one byte read, attempt to read up to the buffer size */
	result = SMPDU_Sock_read(context->sock, &context->read_cmd.cmd[1], SMPD_STDIN_PACKET_SIZE-1, &num_read);
	if (result != SMPD_SUCCESS)
	{
	    num_read = 0;
	    smpd_dbg_printf("SMPDU_Sock_read(%d) failed (%s), assuming %s is closed.\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
	}
	smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));

	/* write stdin to root rsh process */
	total = num_read+1;
	buf = (unsigned char *)context->read_cmd.cmd;
	while (total > 0)
	{
	    result = SMPDU_Sock_write(context->process->in->sock, buf, total, &num_written);
	    if (result != SMPD_SUCCESS)
	    {
		num_read = 0;
		total = 0;
		smpd_dbg_printf("SMPDU_Sock_write(%d) failed (%s), assuming %s is closed.\n",
		    SMPDU_Sock_get_sock_id(context->process->in->sock), get_sock_error_string(result), smpd_get_context_str(context));
	    }
	    else
	    {
		total = total - num_written;
		buf = buf + num_written;
		if (num_written == 0)
		{
		    /* FIXME: what does 0 bytes written mean?
		     * Does it mean that no bytes could be written at that moment
		     * or does it mean that there is an error on the socket?
		     */
		}
	    }
	}
    }
    else
    {
	if (context->read_cmd.stdin_read_offset == SMPD_STDIN_PACKET_SIZE ||
	    context->read_cmd.cmd[context->read_cmd.stdin_read_offset] == '\n')
	{
	    if (context->read_cmd.cmd[context->read_cmd.stdin_read_offset] != '\n')
		smpd_err_printf("truncated command.\n");
	    context->read_cmd.cmd[context->read_cmd.stdin_read_offset] = '\0'; /* remove the \n character */
	    result = smpd_create_command("", -1, -1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a command structure for the stdin command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_init_command(cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to initialize a command structure for the stdin command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    strcpy(cmd_ptr->cmd, context->read_cmd.cmd);
	    if (MPIU_Str_get_int_arg(cmd_ptr->cmd, "src", &cmd_ptr->src) != MPIU_STR_SUCCESS)
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "src", 0);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the default src parameter to the stdin command.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    if (MPIU_Str_get_int_arg(cmd_ptr->cmd, "dest", &cmd_ptr->dest) != MPIU_STR_SUCCESS)
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "dest", 1);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the default dest parameter to the stdin command.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    result = smpd_parse_command(cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("invalid command read from stdin, ignoring: \"%s\"\n", context->read_cmd.cmd);
	    }
	    else
	    {
		if (strcmp(cmd_ptr->cmd_str, "connect") == 0)
		{
		    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "tag", &cmd_ptr->tag) != MPIU_STR_SUCCESS)
		    {
			smpd_dbg_printf("adding tag %d to connect command.\n", smpd_process.cur_tag);
			smpd_add_command_int_arg(cmd_ptr, "tag", smpd_process.cur_tag);
			cmd_ptr->tag = smpd_process.cur_tag;
			smpd_process.cur_tag++;
		    }
		    cmd_ptr->wait = SMPD_TRUE;
		}
		if (strcmp(cmd_ptr->cmd_str, "set") == 0 || strcmp(cmd_ptr->cmd_str, "delete") == 0 ||
		    strcmp(cmd_ptr->cmd_str, "stat") == 0 || strcmp(cmd_ptr->cmd_str, "get") == 0)
		{
		    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "tag", &cmd_ptr->tag) != MPIU_STR_SUCCESS)
		    {
			smpd_dbg_printf("adding tag %d to %s command.\n", smpd_process.cur_tag, cmd_ptr->cmd_str);
			smpd_add_command_int_arg(cmd_ptr, "tag", smpd_process.cur_tag);
			cmd_ptr->tag = smpd_process.cur_tag;
			smpd_process.cur_tag++;
		    }
		    cmd_ptr->wait = SMPD_TRUE;
		}

		smpd_dbg_printf("command read from stdin, forwarding to left_child smpd\n");
		result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the command read from stdin: \"%s\"\n", cmd_ptr->cmd);
		    smpd_free_command(cmd_ptr);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("posted write of command: \"%s\"\n", cmd_ptr->cmd);
	    }
	    context->read_cmd.stdin_read_offset = 0;
	}
	else
	{
	    context->read_cmd.stdin_read_offset++;
	}
    }
    result = SMPDU_Sock_post_read(context->sock, &context->read_cmd.cmd[context->read_cmd.stdin_read_offset], 1, 1, NULL);
    if (result != SMPD_SUCCESS)
    {
	/*
	if (result != SOCK_EOF)
	{
	    smpd_dbg_printf("SMPDU_Sock_post_read failed (%s), assuming %s is closed, calling sock_post_close(%d).\n",
		get_sock_error_string(result), smpd_get_context_str(context), sock_getid(context->sock));
	}
	*/
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_smpd_writing_data_to_stdin"
int smpd_state_smpd_writing_data_to_stdin(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_stdin_write_node_t *node;

    smpd_enter_fn(FCNAME);

    SMPD_UNREFERENCED_ARG(event_ptr);

    node = context->process->stdin_write_list;
    if (node == NULL)
    {
	smpd_err_printf("write completed to process stdin context with no write posted in the list.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_dbg_printf("wrote %d bytes to stdin of rank %d\n", node->length, context->process->rank);
    MPIU_Free(node->buffer);
    MPIU_Free(node);

    context->process->stdin_write_list = context->process->stdin_write_list->next;
    if (context->process->stdin_write_list != NULL)
    {
	context->process->in->write_state = SMPD_WRITING_DATA_TO_STDIN;
	result = SMPDU_Sock_post_write(context->process->in->sock,
	    node->buffer, node->length, node->length, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of %d bytes to stdin for rank %d\n",
		node->length, context->process->rank);
	    smpd_exit_fn(FCNAME);
	}
    }
    else
    {
	context->process->in->write_state = SMPD_IDLE;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_stdouterr"
int smpd_state_reading_stdouterr(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    SMPDU_Size_t num_read;
    char buffer[SMPD_MAX_CMD_LENGTH];
    int num_encoded;
    SMPD_BOOL ends_in_cr = SMPD_FALSE;

    smpd_enter_fn(FCNAME);
    if (context->state == SMPD_CLOSING)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (event_ptr->error != SMPD_SUCCESS)
    {
    /*
	if (event_ptr->error != SMPDU_SOCK_ERR_CONN_CLOSED)
	{
	    smpd_dbg_printf("reading failed(%s), assuming %s is closed.\n",
		get_sock_error_string(event_ptr->error), smpd_get_context_str(context));
	}
    */

	/* Return an error an then handle_sock_op_read will post a close
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	*/

	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_dbg_printf("read from %s\n", smpd_get_context_str(context));

    /* one byte read, attempt to read up to the buffer size */
    result = SMPDU_Sock_read(context->sock, &context->read_cmd.cmd[1], (SMPD_MAX_STDOUT_LENGTH/2)-2, &num_read);
    if (result != SMPD_SUCCESS)
    {
	num_read = 0;
	smpd_dbg_printf("SMPDU_Sock_read(%d) failed (%s), assuming %s is closed.\n",
	    SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /* Use num_read instead of num_read-1 because one byte was already read before increasing the buffer length by one */
    if (context->read_cmd.cmd[num_read] == '\n')
    {
	ends_in_cr = SMPD_TRUE;
    }
    smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));
    if (context->type == SMPD_CONTEXT_STDOUT_RSH || context->type == SMPD_CONTEXT_STDERR_RSH)
    {
	size_t total;
	size_t num_written;
	char *buffer;

	total = num_read+1;
	buffer = context->read_cmd.cmd;
	while (total > 0)
	{
	    num_written = fwrite(buffer, 1, total, context->type == SMPD_CONTEXT_STDOUT_RSH ? stdout : stderr);
	    if (num_written < 1)
	    {
		num_read = 0;
		total = 0;
		smpd_err_printf("fwrite failed: error %d\n", ferror(context->type == SMPD_CONTEXT_STDOUT_RSH ? stdout : stderr));
	    }
	    else
	    {
		total = total - num_written;
		buffer = buffer + num_written;
	    }
	}
    }
    else
    {
	smpd_encode_buffer(buffer, SMPD_MAX_CMD_LENGTH, context->read_cmd.cmd, num_read+1, &num_encoded);
	buffer[num_encoded*2] = '\0';
	/*smpd_dbg_printf("encoded %d characters: %d '%s'\n", num_encoded, strlen(buffer), buffer);*/

	/* create an output command */
	result = smpd_create_command(
	    smpd_get_context_str(context),
	    smpd_process.id, 0 /* output always goes to node 0? */, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an output command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "rank", context->rank);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the rank to the %s command.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* Use the context->first_output_stdout/err flag to indicate that this is the first output since
	 * an end of a line or the very first output of the application.  mpiexec uses this flag when the
	 * -l option is specified to prefix lines with process information.  This flag only handles end of
	 * line situations where the end of line is the last entry in the output.  mpiexec handles end of
	 * line characters in the middle of the output.
	 */
	switch (context->type)
	{
	case SMPD_CONTEXT_STDOUT:
	    if (context->first_output_stdout == SMPD_TRUE)
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "first", 1);
	    }
	    else
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "first", 0);
	    }
	    context->first_output_stdout = ends_in_cr;
	    break;
	case SMPD_CONTEXT_STDERR:
	    if (context->first_output_stderr == SMPD_TRUE)
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "first", 1);
	    }
	    else
	    {
		result = smpd_add_command_int_arg(cmd_ptr, "first", 0);
	    }
	    context->first_output_stderr = ends_in_cr;
	    break;
	default:
	    result = smpd_add_command_int_arg(cmd_ptr, "first", 0);
	    break;
	}
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the first flag to the %s command.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "data", buffer);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the data to the %s command.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* send the stdout command */
	result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the %s command.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }

    /* post a read for the next byte of data */
    result = SMPDU_Sock_post_read(context->sock, &context->read_cmd.cmd, 1, 1, NULL);
    if (result != SMPD_SUCCESS)
    {
	/*
	if (result != SOCK_EOF)
	{
	    smpd_dbg_printf("SMPDU_Sock_post_read failed (%s), assuming %s is closed, calling sock_post_close(%d).\n",
		get_sock_error_string(result), smpd_get_context_str(context), SMPDU_Sock_get_sock_id(context->sock));
	}
	*/
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_cmd_header"
int smpd_state_reading_cmd_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the cmd header on the %s context, %s.\n",
	    smpd_get_context_str(context), get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command header\n");
    context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
    if ((context->read_cmd.length < 1) || (context->read_cmd.length > SMPD_MAX_CMD_LENGTH))
    {
	smpd_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("command header read, posting read for data: %d bytes\n", context->read_cmd.length);
    context->read_cmd.state = SMPD_CMD_READING_CMD;
    context->read_state = SMPD_READING_CMD;
    result = SMPDU_Sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, context->read_cmd.length, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the command string,\nsock error: %s\n", get_sock_error_string(result));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_cmd"
int smpd_state_reading_cmd(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_host_node_t *left, *right, *host_node;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the command, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command\n");
    result = smpd_parse_command(&context->read_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
    context->read_cmd.state = SMPD_CMD_READY;
    result = smpd_handle_command(context);
    if (result == SMPD_SUCCESS)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else if (result == SMPD_CLOSE || result == SMPD_EXITING)
    {
	smpd_dbg_printf("not posting read for another command because %s returned\n",
	    result == SMPD_CLOSE ? "SMPD_CLOSE" : "SMPD_EXITING");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    else if (result == SMPD_EXIT)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* The last process has exited, create a close command to tear down the tree */
	smpd_process.closing = SMPD_TRUE;
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the close command to tear down the job tree.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else if (result == SMPD_CONNECTED)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* mark the node as connected */
	context->connect_to->connected = SMPD_TRUE;
	left = context->connect_to->left;
	right = context->connect_to->right;

	while (left != NULL || right != NULL)
	{
	    if (left != NULL)
	    {
		smpd_dbg_printf("creating connect command for left node\n");
		host_node = left;
		left = NULL;
	    }
	    else
	    {
		smpd_dbg_printf("creating connect command for right node\n");
		host_node = right;
		right = NULL;
	    }
	    smpd_dbg_printf("creating connect command to '%s'\n", host_node->host);
	    /* create a connect command to be sent to the parent */
	    result = smpd_create_command("connect", 0, host_node->parent, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a connect command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    host_node->connect_cmd_tag = cmd_ptr->tag;
	    result = smpd_add_command_arg(cmd_ptr, "host", host_node->host);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", host_node->host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "id", host_node->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", host_node->host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    if (smpd_process.plaintext)
	    {
		/* propagate the plaintext option to the manager doing the connect */
		result = smpd_add_command_arg(cmd_ptr, "plaintext", "yes");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the plaintext parameter to the connect command for host %s\n", host_node->host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the connect command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}

	host_node = smpd_process.host_list;
	while (host_node != NULL)
	{
	    if (host_node->connected == SMPD_FALSE)
	    {
		smpd_dbg_printf("not connected yet: %s not connected\n", host_node->host);
		break;
	    }
	    host_node = host_node->next;
	}
	if (host_node == NULL)
	{
	    context->connect_to = NULL;
	    /* Everyone connected, send the start_dbs command */
	    /* create the start_dbs command to be sent to the first host */
	    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a start_dbs command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }

	    if (context->spawn_context)
	    {
		smpd_dbg_printf("spawn_context found, adding preput values to the start_dbs command.\n");
		result = smpd_add_command_int_arg(cmd_ptr, "npreput", context->spawn_context->npreput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}

		result = smpd_add_command_arg(cmd_ptr, "preput", context->spawn_context->preput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the start_dbs command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
    }
    else if (result == SMPD_DBS_RETURN)
    {
	/*
	printf("SMPD_DBS_RETURN returned, not posting read for the next command.\n");
	fflush(stdout);
	*/
	smpd_exit_fn(FCNAME);
	return SMPD_DBS_RETURN;
    }
    else if (result == SMPD_ABORT)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_post_abort_command("");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post an abort command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_cmd"
int smpd_state_writing_cmd(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr, *cmd_iter;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the command, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote command\n");
    if (context->write_list == NULL)
    {
	smpd_err_printf("data written on a context with no write command posted.\n");
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->write_state = SMPD_IDLE;
    cmd_ptr = context->write_list;
    context->write_list = context->write_list->next;
    smpd_dbg_printf("command written to %s: \"%s\"\n", smpd_get_context_str(context), cmd_ptr->cmd);
    if (strcmp(cmd_ptr->cmd_str, "singinit_info") == 0){
        /* Written singinit_info ... nothing more to be done... close the socket */
        context->state = SMPD_SINGLETON_DONE;
        smpd_free_command(cmd_ptr);
        result = SMPDU_Sock_post_close(context->sock);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("SMPDU_Sock_post_close failed, error = %s\n", get_sock_error_string(result));
            smpd_exit_fn(FCNAME);
            return SMPD_FAIL;
        }
        smpd_exit_fn(FCNAME);
        return SMPD_SUCCESS;
    }
    else if(strcmp(cmd_ptr->cmd_str, "die") == 0){
        /* Sent 'die' command to singleton client. Now close pmi */
        smpd_free_command(cmd_ptr);
        if(context->process){
        if(context->process->pmi){
            smpd_dbg_printf("Closing pmi ...\n");
            result = SMPDU_Sock_post_close(context->process->pmi->sock);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post close on pmi sock\n");
            }
        }
        }
        smpd_exit_fn(FCNAME);
        return SMPD_SUCCESS;
    }
    else if(strcmp(cmd_ptr->cmd_str, "abort_job") == 0){
        if(smpd_process.is_singleton_client){
            /* In the case of a singleton client PM will not be able to "kill" it
            * - so post a read for 'die' cmd
            */
            result = smpd_post_read_command(context);
            if(result != SMPD_SUCCESS){
                smpd_err_printf("Unable to post a read for 'die' command\n");
                if(context->process){
                if(context->process->pmi){
                    smpd_dbg_printf("Closing pmi ...\n");
                    result = SMPDU_Sock_post_close(context->process->pmi->sock);
                    if(result != SMPD_SUCCESS){
                        smpd_err_printf("Unable to post close on pmi sock\n");
                    }
                }
                }
                smpd_exit_fn(FCNAME);
                return SMPD_SUCCESS;
            }
        }
    }
    else if (strcmp(cmd_ptr->cmd_str, "closed") == 0)
    {
	smpd_dbg_printf("closed command written, posting close of the sock.\n");
	smpd_dbg_printf("SMPDU_Sock_post_close(%d)\n", SMPDU_Sock_get_sock_id(context->sock));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'closed' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else if (strcmp(cmd_ptr->cmd_str, "down") == 0)
    {
	smpd_dbg_printf("down command written, posting a close of the %s context\n", smpd_get_context_str(context));
	if (smpd_process.builtin_cmd == SMPD_CMD_RESTART)
	    context->state = SMPD_RESTARTING;
	else
	    context->state = SMPD_EXITING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'down' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn(FCNAME);
	    smpd_exit(0);
	}
	smpd_free_command(cmd_ptr);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd_ptr->cmd_str, "done") == 0)
    {
	smpd_dbg_printf("done command written, posting a close of the %s context\n", smpd_get_context_str(context));
	context->state = SMPD_DONE;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'done' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_free_command(cmd_ptr);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (cmd_ptr->wait)
    {
	/* If this command expects a reply, move it to the wait list */
	smpd_dbg_printf("moving '%s' command to the wait_list.\n", cmd_ptr->cmd_str);
	if (context->wait_list)
	{
	    cmd_iter = context->wait_list;
	    while (cmd_iter->next)
		cmd_iter = cmd_iter->next;
	    cmd_iter->next = cmd_ptr;
	}
	else
	{
	    context->wait_list = cmd_ptr;
	}
	cmd_ptr->next = NULL;
    }
    else
    {
	/* otherwise free the command immediately. */
	result = smpd_free_command(cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to free the written command.\n");
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }

    cmd_ptr = context->write_list;
    if (cmd_ptr)
    {
	context->write_state = SMPD_WRITING_CMD;
	cmd_ptr->iov[0].SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)cmd_ptr->cmd_hdr_str;
	cmd_ptr->iov[0].SMPD_IOV_LEN = SMPD_CMD_HDR_LENGTH;
	cmd_ptr->iov[1].SMPD_IOV_BUF = (SMPD_IOV_BUF_CAST)cmd_ptr->cmd;
	cmd_ptr->iov[1].SMPD_IOV_LEN = cmd_ptr->length;
	smpd_dbg_printf("smpd_handle_written: posting write(%d bytes) for command: \"%s\"\n",
	    cmd_ptr->iov[0].SMPD_IOV_LEN + cmd_ptr->iov[1].SMPD_IOV_LEN, cmd_ptr->cmd);
	result = SMPDU_Sock_post_writev(context->sock, cmd_ptr->iov, 2, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the next command,\nsock error: %s\n", get_sock_error_string(result));
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_smpd_listening"
int smpd_state_smpd_listening(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;
    SMPDU_Sock_t new_sock;
    smpd_context_t *new_context;
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(event_ptr);

    result = SMPDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (smpd_process.service_stop)
    {
	smpd_process.state = SMPD_EXITING;
	if (smpd_process.listener_context)
	{
	    smpd_process.listener_context->state = SMPD_EXITING;
	    smpd_dbg_printf("closing the listener (state = %s).\n", smpd_get_state_string(smpd_process.listener_context->state));
	    result = SMPDU_Sock_post_close(smpd_process.listener_context->sock);
	    smpd_process.listener_context = NULL;
	    if (result == SMPD_SUCCESS)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
		get_sock_error_string(result));
	}
	smpd_free_context(context);
#ifdef HAVE_WINDOWS_H
	SetEvent(smpd_process.hBombDiffuseEvent);
#endif
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("authenticating new connection\n");
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, set, new_sock, -1, &new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the newly accepted sock.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = SMPDU_Sock_set_user_ptr(new_sock, new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    strcpy(phrase, smpd_process.passphrase);
    /* generate the challenge string and the encrypted result */
    if (smpd_gen_authentication_strings(phrase, new_context->pszChallengeResponse, new_context->pszCrypt) != SMPD_SUCCESS)
    {
	smpd_err_printf("failed to generate the authentication strings\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /*
    smpd_dbg_printf("gen_authentication_strings:\n passphrase - %s\n pszStr - %s\n pszCrypt - %s\n",
    phrase, pszStr, pszCrypt);
    */

    /* write the challenge string*/
    smpd_dbg_printf("posting a write of the challenge string: %s\n", new_context->pszChallengeResponse);
    new_context->write_state = SMPD_WRITING_CHALLENGE_STRING;
    result = SMPDU_Sock_post_write(new_sock, new_context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	new_context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(new_sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("posting write of the challenge string failed\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_pmi_listening"
int smpd_state_pmi_listening(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;
    SMPDU_Sock_t new_sock;
    smpd_process_t *iter;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(event_ptr);

    result = SMPDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->state = SMPD_CLOSING;
    result = SMPDU_Sock_post_close(context->sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error closing pmi listener socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    iter = smpd_process.process_list;
    while (iter)
    {
	if (iter->pmi->sock == context->sock)
	{
	    /* process structure found for this listener */
	    iter->pmi->sock = new_sock;
	    iter->context_refcount++;
	    result = SMPDU_Sock_set_user_ptr(new_sock, iter->pmi);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_read_command(iter->pmi);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a read of a command after accepting a pmi connection.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	iter = iter->next;
    }
    result = SMPDU_Sock_post_close(new_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error closing pmi socket not unassociated with any process: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_err_printf("accepted a socket on a listener not associated with a process struct.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_state_pmi_server_listening"
int smpd_state_pmi_server_listening(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;
    SMPDU_Sock_t new_sock;
    smpd_process_t *process;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(event_ptr);

    result = SMPDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_create_process_struct(-1, &process);
    process->next = smpd_process.process_list;
    smpd_process.process_list = process;

    process->pmi->sock = new_sock;
    process->context_refcount++;
    result = SMPDU_Sock_set_user_ptr(new_sock, process->pmi);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_post_read_command(process->pmi);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of a command after accepting a pmi connection.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_mgr_listening"
int smpd_state_mgr_listening(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;
    SMPDU_Sock_t new_sock;
    smpd_context_t *new_context;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(event_ptr);

    result = SMPDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("accepted re-connection\n");
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, set, new_sock, -1, &new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the newly accepted sock.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = SMPDU_Sock_set_user_ptr(new_sock, new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    new_context->access = SMPD_ACCESS_USER_PROCESS;
    new_context->read_state = SMPD_READING_SESSION_HEADER;
    result = SMPDU_Sock_post_read(new_context->sock, new_context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* close the listener */
    smpd_dbg_printf("closing the mgr listener.\n");
    result = SMPDU_Sock_post_close(context->sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a close on the listener sock after accepting the re-connection,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_session_request"
int smpd_state_reading_session_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the session request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read session request: '%s'\n", context->session);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->session, SMPD_SMPD_SESSION_STR) == 0)
    {
	context->target = SMPD_TARGET_SMPD;
	if (smpd_option_on("sspi_protect"))
	{
	    strcpy(context->pwd_request, SMPD_SSPI_REQUEST);
	    context->write_state = SMPD_WRITING_SSPI_REQUEST;
	}
	else
	{
	    if (smpd_process.bPasswordProtect)
	    {
		strcpy(context->pwd_request, SMPD_PWD_REQUEST);
		context->write_state = SMPD_WRITING_PWD_REQUEST;
	    }
	    else
	    {
		strcpy(context->pwd_request, SMPD_NO_PWD_REQUEST);
		context->write_state = SMPD_WRITING_NO_PWD_REQUEST;
	    }
	}
	result = SMPDU_Sock_post_write(context->sock, context->pwd_request, SMPD_MAX_PWD_REQUEST_LENGTH, SMPD_MAX_PWD_REQUEST_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the pwd request '%s',\nsock error: %s\n",
		context->pwd_request, get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else if (strcmp(context->session, SMPD_PROCESS_SESSION_STR) == 0)
    {
	context->target = SMPD_TARGET_PROCESS;
#ifdef HAVE_WINDOWS_H
	if (smpd_process.bService)
	{
	    if (smpd_option_on("jobs_only"))
	    {
		strcpy(context->cred_request, SMPD_CRED_REQUEST_JOB);
	    }
	    else
	    {
		strcpy(context->cred_request, SMPD_CRED_REQUEST);
	    }
	    context->write_state = SMPD_WRITING_CRED_REQUEST;
	}
	else
#endif
	{
	    context->write_state = SMPD_WRITING_NO_CRED_REQUEST;
	    strcpy(context->cred_request, SMPD_NO_CRED_REQUEST);
	}
	result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the credential request string '%s',\nsock error: %s\n",
		context->cred_request, get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else if (strcmp(context->session, SMPD_PMI_SESSION_STR) == 0)
    {
	smpd_process_t *process;

	context->target = SMPD_TARGET_PMI;

	/* create a process struct for this pmi connection */
	smpd_create_process_struct(-1, &process);
	process->next = smpd_process.process_list;
	smpd_process.process_list = process;

	/* remove unused contexts */
	smpd_free_context(process->in);
	smpd_free_context(process->out);
	smpd_free_context(process->err);

	/* replace the pmi context with this context */
	smpd_free_context(process->pmi);
	process->pmi = context;
	context->process = process;
	process->context_refcount++;
	process->local_process = SMPD_FALSE;

	context->type = SMPD_CONTEXT_PMI;

	/* send the id to be used by the pmi context client to send commands */
	context->write_state = SMPD_WRITING_PMI_ID;
	snprintf(context->session, SMPD_SESSION_REQUEST_LEN, "%d", process->id);
	result = SMPDU_Sock_post_write(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the context session id string '%s',\nsock error: %s\n",
		context->session, get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}

	/*
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of a command after accepting a pmi connection.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	*/
    }
    else
    {
	smpd_err_printf("invalid session request: '%s'\n", context->session);
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_smpd_session_request"
int smpd_state_writing_smpd_session_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote smpd session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_PWD_REQUEST;
    result = SMPDU_Sock_post_read(context->sock, context->pwd_request, SMPD_MAX_PWD_REQUEST_LENGTH, SMPD_MAX_PWD_REQUEST_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the pwd request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_process_session_request"
int smpd_state_writing_process_session_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the process session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CRED_REQUEST;
    result = SMPDU_Sock_post_read(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the cred request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_pmi_session_request"
int smpd_state_writing_pmi_session_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the pmi session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote pmi session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_PMI_ID;
    result = SMPDU_Sock_post_read(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the pmi context id,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_pwd_request"
int smpd_state_writing_pwd_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote pwd request: '%s'\n", context->pwd_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SMPD_PASSWORD;
    result = SMPDU_Sock_post_read(context->sock, context->password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the smpd password,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_no_pwd_request"
int smpd_state_writing_no_pwd_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the no pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no pwd request: '%s'\n", context->pwd_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_sspi_request"
int smpd_state_writing_sspi_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the sspi request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote sspi request: '%s'\n", context->pwd_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SSPI_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_pwd_request"
int smpd_state_reading_pwd_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_sspi_type_t type;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read pwd request: '%s'\n", context->pwd_request);
    if (strcmp(context->pwd_request, SMPD_PWD_REQUEST) == 0)
    {
	if (smpd_process.builtin_cmd == SMPD_CMD_ADD_JOB)
	{
	    /* job keys without passwords cannot be added unless the smpd is sspi protected. */
	    smpd_err_printf("unable to save a job key because the smpd is not sspi protected.\n");
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	/* FIXME: the smpd password needs to be able to be set */
	strcpy(context->smpd_pwd, SMPD_DEFAULT_PASSWORD);
	context->write_state = SMPD_WRITING_SMPD_PASSWORD;
	result = SMPDU_Sock_post_write(context->sock, context->smpd_pwd, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the smpd password,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (strcmp(context->pwd_request, SMPD_SSPI_REQUEST) == 0)
    {
	smpd_command_t *cmd_ptr;
	char context_str[20];
	smpd_context_t *dest_context;
	result = smpd_command_destination(0, &dest_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get the context necessary to reach node 0\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	if (smpd_process.builtin_cmd == SMPD_CMD_ASSOCIATE_JOB)
	    context->sspi_type = SMPD_SSPI_DELEGATE;
	else
	    context->sspi_type = SMPD_SSPI_IMPERSONATE;/*SMPD_SSPI_IDENTIFY;*/
	if (dest_context == NULL)
	{
	    /* I am node 0 so handle the command here. */
	    if (smpd_process.builtin_cmd == SMPD_CMD_ASSOCIATE_JOB)
		type = SMPD_SSPI_DELEGATE;
	    else
		type = SMPD_SSPI_IMPERSONATE;/*SMPD_SSPI_IDENTIFY;*/
	    smpd_dbg_printf("calling smpd_sspi_init with host=%s and port=%d\n", smpd_process.console_host, smpd_process.port);
	    result = smpd_sspi_context_init(&context->sspi_context, smpd_process.console_host, smpd_process.port, type);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to initialize an sspi context command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    context->read_state = SMPD_IDLE;
	    context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
	    MPIU_Snprintf(context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", context->sspi_context->buffer_length);
	    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	    if (result == SMPD_SUCCESS)
	    {
		result = SMPD_SUCCESS;
	    }
	    else
	    {
#ifdef HAVE_WINDOWS_H
		smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
		smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
		smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn(FCNAME);
	    return result;
	}

	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;

	result = smpd_create_sspi_client_context(&context->sspi_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an sspi_context.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	/* create a sspi_init command to be sent to root (mpiexec) */
	result = smpd_create_command("sspi_init", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a sspi_init command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	/* FIXME: Instead of encoding a pointer to the context, add an integer id to the context structure and look up the context
	* based on this id from a global list of contexts.
	*/
	MPIU_Snprintf(context_str, 20, "%p", context);
	result = smpd_add_command_arg(cmd_ptr, "sspi_context", context_str);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the context parameter to the sspi_init command for host %s\n", smpd_process.console_host);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	result = smpd_add_command_arg(cmd_ptr, "sspi_host", smpd_process.console_host);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the host parameter to the sspi_init command for host %s\n", smpd_process.console_host);
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "sspi_port", smpd_process.port);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the port parameter to the sspi_init command for host %s\n", smpd_process.console_host);
	    smpd_exit_fn(FCNAME);
	    return result;
	}

	/* post a write of the command */
	result = smpd_post_write_command(dest_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the sspi_init command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}

	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* no password required case */

    if (smpd_process.builtin_cmd == SMPD_CMD_ADD_JOB)
    {
	/* job keys without passwords cannot be added unless the smpd is sspi protected. */
	smpd_err_printf("unable to save a job key because the smpd is not sspi protected.\n");
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	smpd_exit_fn(FCNAME);
	return result;
    }

    result = smpd_generate_session_header(context->session_header, 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = SMPDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_smpd_password"
int smpd_state_reading_smpd_password(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the smpd password, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read smpd password, %d bytes\n", strlen(context->password));
    context->read_state = SMPD_IDLE;
    if (strcmp(context->password, smpd_process.SMPDPassword) == 0)
    {
	context->access = SMPD_ACCESS_ADMIN;
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR);
	context->write_state = SMPD_WRITING_SESSION_ACCEPT;
	result = SMPDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session accepted message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_REJECTED_STR);
	context->write_state = SMPD_WRITING_SESSION_REJECT;
	result = SMPDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session rejected message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_cred_request"
int smpd_state_writing_cred_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote cred request: '%s'\n", context->cred_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CRED_ACK;
    result = SMPDU_Sock_post_read(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the cred ack,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_cred_ack"
int smpd_state_reading_cred_ack(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the cred ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read cred ack: '%s'\n", context->cred_request);
    context->write_state = SMPD_IDLE;
    if (smpd_option_on("jobs_only") && strcmp(context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY))
    {
	/* jobs_only but the client returned something other than SMPD_CRED_ACK_SSPI_JOB_KEY */
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    if (strcmp(context->cred_request, "yes") == 0)
    {
	context->read_state = SMPD_READING_ACCOUNT;
	result = SMPDU_Sock_post_read(context->sock, context->account, SMPD_MAX_ACCOUNT_LENGTH, SMPD_MAX_ACCOUNT_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the account credential,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    else if ( (strcmp(context->cred_request, SMPD_CRED_ACK_SSPI) == 0) || (strcmp(context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY) == 0) )
    {
	if (strcmp(context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY) == 0)
	{
	    context->sspi_type = SMPD_SSPI_IDENTIFY;
	}
	else
	{
	    context->sspi_type = SMPD_SSPI_DELEGATE;
	}
	context->read_state = SMPD_READING_SSPI_HEADER;
	result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    context->state = SMPD_CLOSING;
    result = SMPDU_Sock_post_close(context->sock);
    smpd_exit_fn(FCNAME);
    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_sspi_header"
int smpd_state_reading_sspi_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to read the sspi header, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read sspi header: '%s'\n", context->sspi_header);
    if (context->sspi_context == NULL)
    {
	result = smpd_create_sspi_client_context(&context->sspi_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    context->sspi_context->buffer_length = atoi(context->sspi_header);
    if (context->sspi_context->buffer_length < 1)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("invalid sspi buffer length %d\n", context->sspi_context->buffer_length);
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    if (context->sspi_context->buffer != NULL)
    {
	MPIU_Free(context->sspi_context->buffer);
    }
    context->sspi_context->buffer = MPIU_Malloc(context->sspi_context->buffer_length);
    if (context->sspi_context->buffer == NULL)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to allocate a sspi buffer of length %d\n", context->sspi_context->buffer_length);
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    context->read_state = SMPD_READING_SSPI_BUFFER;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_context->buffer, context->sspi_context->buffer_length, context->sspi_context->buffer_length, NULL);
    if (result != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to post a read of the sspi buffer,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_sspi_buffer"
int smpd_state_reading_sspi_buffer(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    char err_msg[256];
    SECURITY_STATUS sec_result, sec_result_copy;
    SecBufferDesc outbound_descriptor, inbound_descriptor;
    SecBuffer outbound_buffer, inbound_buffer;
    ULONG attr;
    TimeStamp ts;
    SMPD_BOOL first_call = SMPD_FALSE;
    double t1, t2;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to read the sspi buffer, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read sspi buffer\n");
    /* Initialize the security interface */
    if (smpd_process.sec_fn == NULL)
    {
	smpd_dbg_printf("calling InitSecurityInterface\n");
	smpd_process.sec_fn = InitSecurityInterface();
	if (smpd_process.sec_fn == NULL)
	{
	    smpd_err_printf("unable to initialize the sspi interface.\n");
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    /* Query the max token size so the output buffer can be allocated */
    if (context->sspi_context->max_buffer_size == 0)
    {
	PSecPkgInfo info;
	first_call = SMPD_TRUE;
	smpd_dbg_printf("calling QuerySecurityPackageInfo\n");
	sec_result = smpd_process.sec_fn->QuerySecurityPackageInfo(SMPD_SECURITY_PACKAGE, &info);
	if (sec_result != SEC_E_OK)
	{
	    smpd_err_printf("unable to query the security package, error %d.\n", sec_result);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_dbg_printf("%s package, %s, with: max %d byte token, capabilities bitmask 0x%x\n",
	    info->Name, info->Comment, info->cbMaxToken, info->fCapabilities);
	context->sspi_context->max_buffer_size = max(info->cbMaxToken, SMPD_SSPI_MAX_BUFFER_SIZE);
	smpd_dbg_printf("calling FreeContextBuffer\n");
	sec_result = smpd_process.sec_fn->FreeContextBuffer(info);
	if (sec_result != SEC_E_OK)
	{
	    smpd_err_printf("unable to free the security package info buffer, error %d.\n", sec_result);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    if (first_call)
    {
	smpd_dbg_printf("calling AcquireCredentialsHandle\n");
	t1 = PMPI_Wtime();
	sec_result = smpd_process.sec_fn->AcquireCredentialsHandle(NULL, SMPD_SECURITY_PACKAGE, SECPKG_CRED_BOTH/*SECPKG_CRED_INBOUND*/, NULL, NULL, NULL, NULL, &context->sspi_context->credential, &context->sspi_context->expiration_time);
	t2 = PMPI_Wtime();
	smpd_dbg_printf("AcquireCredentialsHandle took %0.6f seconds\n", t2-t1);
	if (sec_result != SEC_E_OK)
	{
	    smpd_err_printf("unable to acquire the security package credential, error %d.\n", sec_result);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }

    inbound_descriptor.ulVersion = SECBUFFER_VERSION;
    inbound_descriptor.cBuffers = 1;
    inbound_descriptor.pBuffers = &inbound_buffer;
    inbound_buffer.BufferType = SECBUFFER_TOKEN;
    inbound_buffer.cbBuffer = context->sspi_context->buffer_length;
    inbound_buffer.pvBuffer = context->sspi_context->buffer;

    outbound_descriptor.ulVersion = SECBUFFER_VERSION;
    outbound_descriptor.cBuffers = 1;
    outbound_descriptor.pBuffers = &outbound_buffer;
    outbound_buffer.BufferType = SECBUFFER_TOKEN;
    outbound_buffer.cbBuffer = context->sspi_context->max_buffer_size;
    if (context->sspi_context->out_buffer != NULL)
    {
	MPIU_Free(context->sspi_context->out_buffer);
    }
    context->sspi_context->out_buffer = MPIU_Malloc(context->sspi_context->max_buffer_size);
    if (context->sspi_context->out_buffer == NULL)
    {
	smpd_err_printf("unable to allocate a sspi buffer of length %d\n", context->sspi_context->max_buffer_size);
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    outbound_buffer.pvBuffer = context->sspi_context->out_buffer;
    smpd_dbg_printf("inbound buffer %d bytes, outbound %d bytes\n", inbound_buffer.cbBuffer, outbound_buffer.cbBuffer);

    smpd_dbg_printf("calling AcceptSecurityContext\n");
    t1 = PMPI_Wtime();
    sec_result = sec_result_copy = smpd_process.sec_fn->AcceptSecurityContext(
	&context->sspi_context->credential,
	first_call ? NULL : &context->sspi_context->context,
	&inbound_descriptor,
	/*0,*/
	/*ASC_REQ_MUTUAL_AUTH | ASC_REQ_DELEGATE,*/
	ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT | ASC_REQ_CONFIDENTIALITY,
	/*ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT | ASC_REQ_CONFIDENTIALITY | ASC_REQ_MUTUAL_AUTH | ASC_REQ_DELEGATE,*/
	/*SECURITY_NATIVE_DREP*/ SECURITY_NETWORK_DREP,
	&context->sspi_context->context,
	&outbound_descriptor,
	&attr, &ts);
    t2 = PMPI_Wtime();
    smpd_dbg_printf("AcceptSecurityContext took %0.6f seconds\n", t2-t1);
    switch (sec_result)
    {
    case SEC_E_OK:
	smpd_dbg_printf("SEC_E_OK\n");
	break;
    case SEC_I_COMPLETE_NEEDED:
	smpd_dbg_printf("SEC_I_COMPLETE_NEEDED\n");
    case SEC_I_COMPLETE_AND_CONTINUE:
	if (sec_result == SEC_I_COMPLETE_AND_CONTINUE)
	{
	    smpd_dbg_printf("SEC_I_COMPLETE_AND_CONTINUE\n");
	}
	smpd_dbg_printf("calling CompleteAuthToken\n");
	sec_result = smpd_process.sec_fn->CompleteAuthToken(&context->sspi_context->context, &outbound_descriptor);
	if (sec_result != SEC_E_OK)
	{
	    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
	    smpd_err_printf("CompleteAuthToken failed with error %d\n", sec_result);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	if (sec_result_copy == SEC_I_COMPLETE_NEEDED)
	    break;
    case SEC_I_CONTINUE_NEEDED:
	if (sec_result_copy == SEC_I_CONTINUE_NEEDED)
	{
	    smpd_dbg_printf("SEC_I_CONTINUE_NEEDED\n");
	}
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_WRITING_SSPI_HEADER;
	MPIU_Snprintf(context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", outbound_buffer.cbBuffer);
	context->sspi_context->out_buffer_length = outbound_buffer.cbBuffer;
	smpd_dbg_printf("continuation buffer of length %d bytes\n", context->sspi_context->out_buffer_length);
	result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
	    smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    default:
	/* error occurred */
	smpd_translate_win_error(sec_result, err_msg, 256, NULL);
	smpd_err_printf("AcceptSecurityContext failed with error %d: %s\n", sec_result, err_msg);
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    /* sspi iterations finished, query whether or not to delegate */
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_DELEGATE_REQUEST;
    MPIU_Strncpy(context->sspi_header, "delegate", SMPD_SSPI_HEADER_LENGTH);
    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
	smpd_err_printf("unable to post a write of the delegate request,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_state_reading_client_sspi_header"
int smpd_state_reading_client_sspi_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to read the sspi header, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read client sspi header: '%s'\n", context->sspi_header);
    if (strcmp(context->sspi_header, "delegate") == 0)
    {
	if (context->target == SMPD_TARGET_SMPD)
	{
	    switch (context->sspi_type)
	    {
	    case SMPD_SSPI_IDENTIFY:
		strcpy(context->sspi_header, "identify");
		break;
	    case SMPD_SSPI_IMPERSONATE:
		strcpy(context->sspi_header, "no");
		break;
	    default:
		strcpy(context->sspi_header, "yes");
		break;
	    }
	}
	else
	{
	    if (smpd_process.use_sspi_job_key)
	    {
		strcpy(context->sspi_header, "key");
	    }
	    else
	    {
		/* sspi iterations finished, delegation query received */
		/* send a yes or no reply */
		strcpy(context->sspi_header, (smpd_process.use_delegation) ? "yes" : "no");
	    }
	}
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_WRITING_DELEGATE_REQUEST_RESULT;
	result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else
    {
	/* sspi iteration buffer length sent, receive the buffer */
	context->sspi_context->buffer_length = atoi(context->sspi_header);
	if (context->sspi_context->buffer_length < 1)
	{
	    /* FIXME: Add code to cleanup sspi structures */
	    smpd_err_printf("invalid sspi buffer length %d\n", context->sspi_context->buffer_length);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	if (context->sspi_context->buffer != NULL)
	{
	    MPIU_Free(context->sspi_context->buffer);
	}
	context->sspi_context->buffer = MPIU_Malloc(context->sspi_context->buffer_length);
	if (context->sspi_context->buffer == NULL)
	{
	    /* FIXME: Add code to cleanup sspi structures */
	    smpd_err_printf("unable to allocate a sspi buffer of length %d\n", context->sspi_context->buffer_length);
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	context->read_state = SMPD_READING_CLIENT_SSPI_BUFFER;
	result = SMPDU_Sock_post_read(context->sock, context->sspi_context->buffer, context->sspi_context->buffer_length, context->sspi_context->buffer_length, NULL);
	if (result != SMPD_SUCCESS)
	{
	    /* FIXME: Add code to cleanup sspi structures */
	    smpd_err_printf("unable to post a read of the sspi buffer,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_client_sspi_buffer"
int smpd_state_reading_client_sspi_buffer(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    char context_str[20];
    smpd_context_t *dest_context;
    char *encoded = NULL;
    int encoded_max_length;
    int num_encoded;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to read the sspi buffer, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read client sspi buffer\n");

    result = smpd_command_destination(0, &dest_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get the context necessary to reach node 0\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    if (dest_context == NULL)
    {
	/* I am node 0 so handle the command here. */
	result = smpd_sspi_context_iter(context->sspi_context->id, &context->sspi_context->buffer, &context->sspi_context->buffer_length);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to process the sspi iteration buffer\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	if (context->sspi_context->buffer_length == 0)
	{
	    /* FIXME: This assumes that the server knows to post a write of the delegate command because it knows that no buffer will be returned by the iter command */
	    context->write_state = SMPD_IDLE;
	    context->read_state = SMPD_READING_CLIENT_SSPI_HEADER;
	    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		/* FIXME: Add code to cleanup sspi structures */
		smpd_err_printf("unable to post a read of the client sspi header,\nsock error: %s\n", get_sock_error_string(result));
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	}
	else if (context->sspi_context->buffer_length > 0)
	{
	    context->read_state = SMPD_IDLE;
	    context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
	    MPIU_Snprintf(context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", context->sspi_context->buffer_length);
	    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
#ifdef HAVE_WINDOWS_H
		smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
		smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
		smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	}
	else
	{
	    smpd_err_printf("invalid buffer length returned: %d\n", context->sspi_context->buffer_length);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* create an sspi_iter command and send the sspi buffer to the root */
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;

    /* create a sspi_iter command to be sent to root (mpiexec) */
    result = smpd_create_command("sspi_iter", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a sspi_iter command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    /* FIXME: Instead of encoding a pointer to the context, add an integer id to the context structure and look up the context
     * based on this id from a global list of contexts.
     */
    MPIU_Snprintf(context_str, 20, "%p", context);
    result = smpd_add_command_arg(cmd_ptr, "sspi_context", context_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the sspi_context parameter to the sspi_iter command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "sspi_id", context->sspi_context->id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the sspi_id parameter to the sspi_iter command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    encoded_max_length = context->sspi_context->buffer_length * 2 + 10;
    encoded = (char*)MPIU_Malloc(encoded_max_length);
    if (encoded == NULL)
    {
	smpd_err_printf("unable to allocate a buffer to encode the sspi buffer.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_encode_buffer(encoded, encoded_max_length, context->sspi_context->buffer, context->sspi_context->buffer_length, &num_encoded);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to encode the sspi buffer.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    if (num_encoded != context->sspi_context->buffer_length)
    {
	smpd_err_printf("unable to encode the sspi buffer, only %d bytes of %d encoded.\n", num_encoded, context->sspi_context->buffer_length);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_arg(cmd_ptr, "data", encoded);
    MPIU_Free(encoded);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the data parameter to the sspi_iter command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }

    /* post a write of the command */
    result = smpd_post_write_command(dest_context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the sspi_iter command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_delegate_request"
int smpd_state_writing_delegate_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
#ifdef HAVE_WINDOWS_H
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
	smpd_err_printf("unable to write the delegate request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_DELEGATE_REQUEST_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
#ifdef HAVE_WINDOWS_H
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
	smpd_err_printf("unable to post a read of the delegate request result,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_delegate_request_result"
int smpd_state_reading_delegate_request_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    char err_msg[256];
    const char *result_str = SMPD_SUCCESS_STR;
    SECURITY_STATUS sec_result;
    HANDLE user_handle;
    BOOL duplicate_result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
	smpd_err_printf("unable to read the delegate request result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("delegate request result: '%s'\n", context->sspi_header);

    if (context->sspi_type == SMPD_SSPI_IDENTIFY || (strcmp(context->sspi_header, "identify") == 0))
    {
	context->sspi_type = SMPD_SSPI_IDENTIFY;
	smpd_dbg_printf("calling ImpersonateSecurityContext\n");
	sec_result = smpd_process.sec_fn->ImpersonateSecurityContext(&context->sspi_context->context);
	/* revert must be called before any smpd_dbg_printfs will work */
	smpd_get_user_name(context->account, context->domain, context->full_domain);
	if (sec_result == SEC_E_OK)
	{
	    smpd_process.sec_fn->RevertSecurityContext(&context->sspi_context->context);
	    smpd_dbg_printf("impersonated user: '%s'\n", context->account);
	}
	else
	{
	    smpd_err_printf("ImpersonateSecurityContext failed: %d\n", sec_result);
	}

	if (strcmp(context->sspi_header, "key") != 0)
	{
	    /* Error: identify must be coupled with an sspi job key */
	    context->read_state = SMPD_IDLE;
	    context->write_state = SMPD_WRITING_IMPERSONATE_RESULT;
	    MPIU_Strncpy(context->sspi_header, SMPD_FAIL_STR, SMPD_SSPI_HEADER_LENGTH);
	    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the impersonate result,\nsock error: %s\n", get_sock_error_string(result));
		context->state = SMPD_CLOSING;
		result = SMPDU_Sock_post_close(context->sock);
		smpd_exit_fn(FCNAME);
		return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	    }

	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	context->read_state = SMPD_READING_SSPI_JOB_KEY;
	result = SMPDU_Sock_post_read(context->sock, context->sspi_job_key, SMPD_SSPI_JOB_KEY_LENGTH, SMPD_SSPI_JOB_KEY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the sspi job key,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (context->target == SMPD_TARGET_SMPD && (strcmp(context->sspi_header, "no") == 0))
    {
	context->sspi_type = SMPD_SSPI_IMPERSONATE;
	sec_result = smpd_process.sec_fn->ImpersonateSecurityContext(&context->sspi_context->context);
	/* revert must be called before any smpd_dbg_printfs will work */
	smpd_get_user_name(context->account, context->domain, context->full_domain);

	if (sec_result == SEC_E_OK)
	{
	    /* verify local admin */
	    BOOL b = FALSE;
	    int error;
	    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	    PSID AdministratorsGroup;
	    context->access = SMPD_ACCESS_NONE;
	    result_str = SMPD_FAIL_STR;
	    if (AllocateAndInitializeSid(
		&NtAuthority,
		2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&AdministratorsGroup))
	    {
		if (CheckTokenMembership(NULL, AdministratorsGroup, &b)) 
		{
		    smpd_process.sec_fn->RevertSecurityContext(&context->sspi_context->context);
		    if (b)
		    {
			smpd_dbg_printf("allowing admin access to smpd\n");
			context->access = SMPD_ACCESS_ADMIN;
			result_str = SMPD_SUCCESS_STR;
		    }
		    else
		    {
			smpd_dbg_printf("CheckTokenMembership returned %s is not an administrator, denying admin access to smpd.\n", context->account);
		    }
		}
		else
		{
		    error = GetLastError();
		    smpd_process.sec_fn->RevertSecurityContext(&context->sspi_context->context);
		    smpd_dbg_printf("CheckTokenMembership returned false, %d, denying admin access to smpd.\n", error);
		}
		FreeSid(AdministratorsGroup); 
	    }
	    else
	    {
		smpd_process.sec_fn->RevertSecurityContext(&context->sspi_context->context);
		smpd_err_printf("AllocateAndInitializeSid failed: %d\n", GetLastError());
	    }
	    smpd_dbg_printf("impersonated user: '%s'\n", context->account);
	}
	else
	{
	    smpd_err_printf("ImpersonateSecurityContext failed: %d\n", sec_result);
	    result_str = SMPD_FAIL_STR;
	}
	/* Clean up the sspi structures */
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);

	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_WRITING_IMPERSONATE_RESULT;
	MPIU_Strncpy(context->sspi_header, result_str, SMPD_SSPI_HEADER_LENGTH);
	result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the impersonate result,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}

	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("calling QuerySecurityContextToken\n");
    sec_result = smpd_process.sec_fn->QuerySecurityContextToken(&context->sspi_context->context, &context->sspi_context->user_handle);
    if (sec_result == SEC_E_OK)
    {
	/* Create a primary token to be used to start the manager process */
	if (strcmp(context->sspi_header, "yes") == 0)
	{
	    /* full delegation requested */
	    smpd_dbg_printf("calling DuplicateTokenEx with SecurityDelegation\n");
	    duplicate_result = DuplicateTokenEx(context->sspi_context->user_handle, MAXIMUM_ALLOWED, NULL, SecurityDelegation, TokenPrimary, &user_handle);
	    if (context->target == SMPD_TARGET_SMPD)
	    {
		/* smpd targets need the user token and the user name */
		/* so get the user name here */
		sec_result = smpd_process.sec_fn->ImpersonateSecurityContext(&context->sspi_context->context);
		smpd_get_user_name(context->account, context->domain, context->full_domain);
		if (sec_result == SEC_E_OK)
		{
		    smpd_process.sec_fn->RevertSecurityContext(&context->sspi_context->context);
		    smpd_dbg_printf("impersonated user: '%s'\n", context->account);
		}
		else
		{
		    smpd_err_printf("ImpersonateSecurityContext failed: %d\n", sec_result);
		    result_str = SMPD_FAIL_STR;
		}
	    }
	}
	else
	{
	    /* impersonate only */
	    smpd_dbg_printf("calling DuplicateTokenEx with SecurityImpersonation\n");
	    duplicate_result = DuplicateTokenEx(context->sspi_context->user_handle, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &user_handle);
	}
	if (duplicate_result)
	{
	    CloseHandle(context->sspi_context->user_handle);
	    context->sspi_context->user_handle = user_handle;
	    smpd_dbg_printf("duplicated user token: %p\n", user_handle);
	}
	else
	{
	    result = GetLastError();
	    smpd_translate_win_error(result, err_msg, 256, NULL);
	    smpd_err_printf("DuplicateTokenEx failed: error %d, %s\n", result, err_msg);
	    CloseHandle(context->sspi_context->user_handle);
	    result_str = SMPD_FAIL_STR;
	}
    }
    else
    {
	result_str = SMPD_FAIL_STR;
    }

    /* Clean up the sspi structures */
    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_IMPERSONATE_RESULT;
    MPIU_Strncpy(context->sspi_header, result_str, SMPD_SSPI_HEADER_LENGTH);
    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the impersonate result,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_state_reading_sspi_job_key"
int smpd_state_reading_sspi_job_key(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    const char *result_str = SMPD_SUCCESS_STR;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
	smpd_err_printf("unable to read the delegate request result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("job key: '%s'\n", context->sspi_job_key);

    result = smpd_lookup_job_key(context->sspi_job_key, context->account, &context->sspi_context->user_handle, &context->sspi_context->job);
    if (result == SMPD_SUCCESS)
    {
	context->sspi_context->close_handle = SMPD_FALSE;
    }
    else
    {
	result_str = SMPD_FAIL_STR;
    }

    /* Clean up the sspi structures */
    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_IMPERSONATE_RESULT;
    result = MPIU_Strncpy(context->sspi_header, result_str, SMPD_SSPI_HEADER_LENGTH);
    result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the impersonate result,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
#else
    smpd_enter_fn(FCNAME);
    smpd_err_printf("function not implemented.\n");
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_state_writing_delegate_request_result"
int smpd_state_writing_delegate_request_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the delegate request result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    if (smpd_process.use_sspi_job_key)
    {
	context->write_state = SMPD_WRITING_SSPI_JOB_KEY;
	result = SMPDU_Sock_post_write(context->sock, smpd_process.job_key/*context->sspi_job_key*/, SMPD_SSPI_JOB_KEY_LENGTH, SMPD_SSPI_JOB_KEY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the sspi job key,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* read the result */
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_IMPERSONATE_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the impersonation result,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_sspi_job_key"
int smpd_state_writing_sspi_job_key(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the delegate request result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    /* read the result */
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_IMPERSONATE_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the impersonation result,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_impersonate_result"
int smpd_state_reading_impersonate_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the impersonation result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("result of impersonation: %s\n", context->sspi_header);
    if (strcmp(context->sspi_header, SMPD_SUCCESS_STR))
    {
	/* impersonation failed */
	/* close the context */
	smpd_dbg_printf("impersonation failed.\n");
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (smpd_process.left_context == context)
	    smpd_process.left_context = NULL;
	/* abort the job */
	smpd_post_abort_command("Impersonation failed");
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    else
    {
	/* impersonation succeeded */
	if (context->target == SMPD_TARGET_SMPD)
	{
	    result = smpd_generate_session_header(context->session_header, 1);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to generate a session header.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    context->read_state = SMPD_IDLE;
	    context->write_state = SMPD_WRITING_SESSION_HEADER;
	    result = SMPDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	else
	{
	    context->write_state = SMPD_IDLE;
	    context->read_state = SMPD_READING_RECONNECT_REQUEST;
	    result = SMPDU_Sock_post_read(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a read of the re-connect request,\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_impersonate_result"
int smpd_state_writing_impersonate_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the impersonation result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    if (strcmp(context->sspi_header, SMPD_SUCCESS_STR))
    {
	/* impersonation failed, close the context */
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    if (context->target == SMPD_TARGET_SMPD)
    {
	switch (context->sspi_type)
	{
	case SMPD_SSPI_IDENTIFY:
	    break;
	case SMPD_SSPI_IMPERSONATE:
	    break;
	default:
	    context->access = SMPD_ACCESS_USER;
	    break;
	}
	context->write_state = SMPD_IDLE;
	context->read_state = SMPD_READING_SESSION_HEADER;
	result = SMPDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    /* impersonation succeeded, launch the manager process */
#ifdef HAVE_WINDOWS_H
    result = smpd_start_win_mgr(context, SMPD_TRUE);
    if (result != SMPD_SUCCESS)
    {
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a failed win_mgr, error:\n%s\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
#else
    /* If the sspi functions were implemented under unix then the fork stuff would go here. */
#endif
    /* post a write of the reconnect request */
    smpd_dbg_printf("smpd writing reconnect request: port %s\n", context->port_str);
    context->write_state = SMPD_WRITING_RECONNECT_REQUEST;
    result = SMPDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_sspi_header"
int smpd_state_writing_sspi_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to write the sspi header, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote sspi header: %s.\n", context->sspi_header);

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_SSPI_BUFFER;
    result = SMPDU_Sock_post_write(context->sock, context->sspi_context->out_buffer, context->sspi_context->out_buffer_length, context->sspi_context->out_buffer_length, NULL);
    if (result != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to post a write of the sspi buffer,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_sspi_buffer"
int smpd_state_writing_sspi_buffer(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to write the sspi buffer, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote sspi buffer.\n");

    if (context->sspi_context->out_buffer != NULL)
    {
	MPIU_Free(context->sspi_context->out_buffer);
	context->sspi_context->out_buffer = NULL;
	context->sspi_context->out_buffer_length = 0;
    }
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SSPI_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to post a read of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_client_sspi_header"
int smpd_state_writing_client_sspi_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to write the client sspi header, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote client sspi header: %s.\n", context->sspi_header);

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_CLIENT_SSPI_BUFFER;
    result = SMPDU_Sock_post_write(context->sock, context->sspi_context->buffer, context->sspi_context->buffer_length, context->sspi_context->buffer_length, NULL);
    if (result != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to post a write of the client sspi buffer,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_client_sspi_buffer"
int smpd_state_writing_client_sspi_buffer(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to write the client sspi buffer, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote sspi buffer.\n");

    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CLIENT_SSPI_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	/* FIXME: Add code to cleanup sspi structures */
	smpd_err_printf("unable to post a read of the client sspi header,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_cred_ack_sspi"
int smpd_state_writing_cred_ack_sspi(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    char context_str[20];
    smpd_context_t *dest_context;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request sspi ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request sspi ack.\n");

    result = smpd_command_destination(0, &dest_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get the context necessary to reach node 0\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    if (dest_context == NULL)
    {
	/* I am node 0 so handle the command here. */
	smpd_dbg_printf("calling smpd_sspi_init with host=%s and port=%d\n", context->connect_to->host, smpd_process.port);
	result = smpd_sspi_context_init(&context->sspi_context, context->connect_to->host, smpd_process.port, SMPD_SSPI_DELEGATE);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to initialize an sspi context command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
	MPIU_Snprintf(context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", context->sspi_context->buffer_length);
	result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result == SMPD_SUCCESS)
	{
	    result = SMPD_SUCCESS;
	}
	else
	{
#ifdef HAVE_WINDOWS_H
	    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
	    smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return result;
    }

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;

    result = smpd_create_sspi_client_context(&context->sspi_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an sspi_context.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    /* create a sspi_init command to be sent to root (mpiexec) */
    result = smpd_create_command("sspi_init", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a sspi_init command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    /* FIXME: Instead of encoding a pointer to the context, add an integer id to the context structure and look up the context
     * based on this id from a global list of contexts.
     */
    MPIU_Snprintf(context_str, 20, "%p", context);
    result = smpd_add_command_arg(cmd_ptr, "sspi_context", context_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the context parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_arg(cmd_ptr, "sspi_host", context->connect_to->host);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the host parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "sspi_port", smpd_process.port);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the port parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }

    /* post a write of the command */
    result = smpd_post_write_command(dest_context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the sspi_init command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_cred_ack_sspi_job_key"
int smpd_state_writing_cred_ack_sspi_job_key(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    char context_str[20];
    smpd_context_t *dest_context;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request sspi ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request sspi job key ack.\n");

    result = smpd_command_destination(0, &dest_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get the context necessary to reach node 0\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    if (dest_context == NULL)
    {
	/* I am node 0 so handle the command here. */
	smpd_dbg_printf("calling smpd_sspi_init with host=%s and port=%d\n", context->connect_to->host, smpd_process.port);
	result = smpd_sspi_context_init(&context->sspi_context, context->connect_to->host, smpd_process.port, SMPD_SSPI_IDENTIFY);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to initialize an sspi context command.\n");
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_WRITING_CLIENT_SSPI_HEADER;
	MPIU_Snprintf(context->sspi_header, SMPD_SSPI_HEADER_LENGTH, "%d", context->sspi_context->buffer_length);
	result = SMPDU_Sock_post_write(context->sock, context->sspi_header, SMPD_SSPI_HEADER_LENGTH, SMPD_SSPI_HEADER_LENGTH, NULL);
	if (result == SMPD_SUCCESS)
	{
	    result = SMPD_SUCCESS;
	}
	else
	{
#ifdef HAVE_WINDOWS_H
	    smpd_process.sec_fn->DeleteSecurityContext(&context->sspi_context->context);
	    smpd_process.sec_fn->FreeCredentialsHandle(&context->sspi_context->credential);
#endif
	    smpd_err_printf("unable to post a write of the sspi header,\nsock error: %s\n", get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    result = (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return result;
    }

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;

    result = smpd_create_sspi_client_context(&context->sspi_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an sspi_context.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    /* create a sspi_init command to be sent to root (mpiexec) */
    result = smpd_create_command("sspi_init", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a sspi_init command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }
    /* FIXME: Instead of encoding a pointer to the context, add an integer id to the context structure and look up the context
     * based on this id from a global list of contexts.
     */
    MPIU_Snprintf(context_str, 20, "%p", context);
    result = smpd_add_command_arg(cmd_ptr, "sspi_context", context_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the context parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_arg(cmd_ptr, "sspi_host", context->connect_to->host);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the host parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "sspi_port", smpd_process.port);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the port parameter to the sspi_init command for host %s\n", context->connect_to->host);
	smpd_exit_fn(FCNAME);
	return result;
    }

    /* post a write of the command */
    result = smpd_post_write_command(dest_context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the sspi_init command.\n");
	smpd_exit_fn(FCNAME);
	return result;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}


#undef FCNAME
#define FCNAME "smpd_state_writing_cred_ack_yes"
int smpd_state_writing_cred_ack_yes(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request yes ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request yes ack.\n");

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_ACCOUNT;
    result = SMPDU_Sock_post_write(context->sock, context->account, SMPD_MAX_ACCOUNT_LENGTH, SMPD_MAX_ACCOUNT_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the account '%s',\nsock error: %s\n",
	    context->account, get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_cred_ack_no"
int smpd_state_writing_cred_ack_no(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    char *host_ptr;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request no ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request no ack.\n");

    /* insert code here to handle failed connection attempt */
    smpd_process.left_context = NULL;
    if (smpd_process.do_console && smpd_process.console_host[0] != '\0')
	host_ptr = smpd_process.console_host;
    else if (context->connect_to && context->connect_to->host[0] != '\0')
	host_ptr = context->connect_to->host;
    else if (context->host[0] != '\0')
	host_ptr = context->host;
    else
	host_ptr = NULL;
    if (host_ptr)
	result = smpd_post_abort_command("Unable to connect to %s", host_ptr);
    else
	result = smpd_post_abort_command("connection failed");

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    context->state = SMPD_CLOSING;
    result = SMPDU_Sock_post_close(context->sock);
    smpd_exit_fn(FCNAME);
    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_account"
int smpd_state_reading_account(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the account, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read account: '%s'\n", context->account);
    context->read_state = SMPD_READING_PASSWORD;
    result = SMPDU_Sock_post_read(context->sock, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the password credential,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_password"
int smpd_state_reading_password(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    char *iter;
    char decrypted[SMPD_MAX_PASSWORD_LENGTH];
    int length;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the password, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }

    /* decrypt the password */
    length = SMPD_MAX_PASSWORD_LENGTH;
    result = smpd_decrypt_data(context->encrypted_password+1, (int)strlen(context->encrypted_password+1), decrypted, &length);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to decrypt the password\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (length >= 0 && length < SMPD_MAX_PASSWORD_LENGTH)
    {
	decrypted[length] = '\0';
    }
    else
    {
	smpd_err_printf("unable to decrypt the password, invalid length of %d bytes decrypted returned.\n", length);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* skip over the entropy prefix */
    iter = decrypted;
    while (*iter != ' ' && *iter != '\0')
	iter++;
    if (*iter == ' ')
	iter++;
    MPIU_Strncpy(context->password, iter, SMPD_MAX_PASSWORD_LENGTH);
    smpd_dbg_printf("read password, %d bytes\n", strlen(context->password));
#ifdef HAVE_WINDOWS_H
    /* launch the manager process */
    result = smpd_start_win_mgr(context, SMPD_FALSE);
    if (result == SMPD_SUCCESS)
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR);
	context->write_state = SMPD_WRITING_PROCESS_SESSION_ACCEPT;
	result = SMPDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the process session accepted message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_REJECTED_STR);
	context->write_state = SMPD_WRITING_PROCESS_SESSION_REJECT;
	result = SMPDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session rejected message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
#else
    /* post a write of the noreconnect request */
    smpd_dbg_printf("smpd writing noreconnect request\n");
    context->write_state = SMPD_WRITING_NO_RECONNECT_REQUEST;
    strcpy(context->port_str, SMPD_NO_RECONNECT_PORT_STR);
    result = SMPDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_account"
int smpd_state_writing_account(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    char buffer[SMPD_MAX_PASSWORD_LENGTH];

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the account, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote account: '%s'\n", context->account);

    /* encrypt the password and a prefix to introduce variability to the encrypted blob */
    /* The prefix used is the challenge response string */
    if (context->encrypted_password[0] == '\0')
    {
	context->encrypted_password[0] = SMPD_ENCRYPTED_PREFIX;
	MPIU_Snprintf(buffer, SMPD_MAX_PASSWORD_LENGTH, "%s %s", smpd_process.encrypt_prefix, context->password);
	result = smpd_encrypt_data(buffer, (int)strlen(buffer), context->encrypted_password+1, SMPD_MAX_PASSWORD_LENGTH-1);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to encrypt the user password.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    context->write_state = SMPD_WRITING_PASSWORD;
    result = SMPDU_Sock_post_write(context->sock, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the password,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_password"
int smpd_state_writing_password(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the password, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote password\n");
    context->read_state = SMPD_READING_PROCESS_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the process session result,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_no_cred_request"
int smpd_state_writing_no_cred_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the no cred request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no cred request: '%s'\n", context->cred_request);
#ifdef HAVE_WINDOWS_H
    /* launch the manager process */
    result = smpd_start_win_mgr(context, SMPD_FALSE);
    if (result != SMPD_SUCCESS)
    {
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a failed win_mgr, error:\n%s\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    /* post a write of the reconnect request */
    smpd_dbg_printf("smpd writing reconnect request: port %s\n", context->port_str);
    context->write_state = SMPD_WRITING_RECONNECT_REQUEST;
    result = SMPDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
#else
    /* post a write of the noreconnect request */
    smpd_dbg_printf("smpd writing noreconnect request\n");
    context->write_state = SMPD_WRITING_NO_RECONNECT_REQUEST;
    strcpy(context->port_str, SMPD_NO_RECONNECT_PORT_STR);
    result = SMPDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_cred_request"
int smpd_state_reading_cred_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the cred request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read cred request: '%s'\n", context->cred_request);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->cred_request, SMPD_CRED_REQUEST) == 0)
    {
	if (smpd_process.use_sspi)
	{
	    strcpy(context->cred_request, SMPD_CRED_ACK_SSPI);
	    context->write_state = SMPD_WRITING_CRED_ACK_SSPI;
	    context->read_state = SMPD_IDLE;
	    result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the cred request sspi ack.\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    return SMPD_SUCCESS;
	}

	if (smpd_process.use_sspi_job_key)
	{
	    strcpy(context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY);
	    context->write_state = SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY;
	    context->read_state = SMPD_IDLE;
	    result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the cred request sspi ack.\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    return SMPD_SUCCESS;
	}

#ifdef HAVE_WINDOWS_H
	if (smpd_process.UserAccount[0] == '\0')
	{
	    if (smpd_process.logon || 
		(!smpd_get_cached_password(context->account, context->password) &&
		!smpd_read_password_from_registry(smpd_process.user_index, context->account, context->password)))
	    {
		if (smpd_process.id > 0 && smpd_process.parent_context && smpd_process.parent_context->sock != SMPDU_SOCK_INVALID_SOCK)
		{
		    result = smpd_create_command("cred_request", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a command structure for the cred_request command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add host=%s to the cred_request command.\n", context->connect_to->host);
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    cmd_ptr->context = context;
		    result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the cred_request command.\n");
			smpd_exit_fn(FCNAME);
			return result;
		    }
		    smpd_exit_fn(FCNAME);
		    return SMPD_SUCCESS;
		}
		if (smpd_process.id == 0 && smpd_process.credentials_prompt)
		{
		    fprintf(stderr, "User credentials needed to launch processes:\n");
		    smpd_get_account_and_password(context->account, context->password);
		    smpd_cache_password(context->account, context->password);
		}
		else
		{
		    /*smpd_post_abort_command("User credentials needed to launch processes.\n");*/
		    strcpy(context->account, "invalid account");
		}
	    }
	    else if (!smpd_process.logon)
	    {
		/* This will re-cache cached passwords but I can't think of a way to determine the difference between
		a cached and non-cached password retrieval. */
		/*if (password_read_from_registry)*/
		smpd_cache_password(context->account, context->password);
	    }
	}
	else
	{
	    strcpy(context->account, smpd_process.UserAccount);
	    strcpy(context->password, smpd_process.UserPassword);
	}
#else
	if (smpd_process.UserAccount[0] == '\0')
	{
	    if (smpd_process.id > 0 && smpd_process.parent_context && smpd_process.parent_context->sock != SMPDU_SOCK_INVALID_SOCK)
	    {
		result = smpd_create_command("cred_request", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a command structure for the cred_request command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add host=%s to the cred_request command.\n", context->connect_to->host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		cmd_ptr->context = context;
		result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the cred_request command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    if (smpd_process.credentials_prompt)
	    {
		fprintf(stderr, "User credentials needed to launch processes:\n");
		smpd_get_account_and_password(context->account, context->password);
	    }
	    else
	    {
		/*smpd_post_abort_command("User credentials needed to launch processes.\n");*/
		strcpy(context->account, "invalid account");
	    }
	}
	else
	{
	    strcpy(context->account, smpd_process.UserAccount);
	    strcpy(context->password, smpd_process.UserPassword);
	}
#endif
	if (strcmp(context->account, "invalid account") == 0)
	{
	    /*smpd_err_printf("Attempting to create a session with an smpd that requires credentials without having obtained any credentials.\n");*/
	    if (context->connect_to != NULL)
	    {
		smpd_err_printf("Credentials required to connect to the process manager on %s.  Please use \"mpiexec -register\" or \"mpiexec -logon ...\" to provide user credentials.\n", context->connect_to->host);
	    }
	    else
	    {
		smpd_err_printf("Credentials required to connect to the process manager.  Please use \"mpiexec -register\" or \"mpiexec -logon ...\" to provide user credentials.\n");
	    }
	    strcpy(context->cred_request, "no");
	    context->write_state = SMPD_WRITING_CRED_ACK_NO;
	    context->read_state = SMPD_IDLE;
	    result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	strcpy(context->cred_request, "yes");
	context->write_state = SMPD_WRITING_CRED_ACK_YES;
	context->read_state = SMPD_IDLE;
	result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the cred request yes ack.\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }
    if (strcmp(context->cred_request, SMPD_CRED_REQUEST_JOB) == 0)
    {
	if (smpd_process.use_sspi_job_key)
	{
	    strcpy(context->cred_request, SMPD_CRED_ACK_SSPI_JOB_KEY);
	    context->write_state = SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY;
	    context->read_state = SMPD_IDLE;
	    result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the cred request sspi ack.\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    return SMPD_SUCCESS;
	}
	/* smpd requested(requires) a job but no job key specified. */
	strcpy(context->cred_request, "no");
	context->write_state = SMPD_WRITING_CRED_ACK_NO;
	context->read_state = SMPD_IDLE;
	result = SMPDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    context->read_state = SMPD_READING_RECONNECT_REQUEST;
    result = SMPDU_Sock_post_read(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the re-connect request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_reconnect_request"
int smpd_state_writing_reconnect_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote reconnect request: '%s'\n", context->port_str);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = SMPDU_Sock_post_close(context->sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a close on sock(%d) after reconnect request written, error:\n%s\n",
	    SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_no_reconnect_request"
int smpd_state_writing_no_reconnect_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the no re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no reconnect request: '%s'\n", context->port_str);
#ifdef HAVE_WINDOWS_H
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
#else
    /* fork the manager process */
    result = smpd_start_unx_mgr(context);
    if (result != SMPD_SUCCESS)
    {
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a failed unx_mgr, error:\n%s\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (smpd_process.root_smpd)
    {
	smpd_dbg_printf("root closing sock(%d) after fork.\n", SMPDU_Sock_get_sock_id(context->sock));
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a unx_mgr, error:\n%s\n",
		SMPDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_dbg_printf("child closing listener sock(%d) after fork.\n", SMPDU_Sock_get_sock_id(smpd_process.listener_context->sock));
	/* close the listener in the child */
	smpd_process.listener_context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(smpd_process.listener_context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on the listener,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/* post a read of the session header */
	context->read_state = SMPD_READING_SESSION_HEADER;
	result = SMPDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_reconnect_request"
int smpd_state_reading_reconnect_request(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read re-connect request: '%s'\n", context->port_str);
    if (strcmp(context->port_str, SMPD_NO_RECONNECT_PORT_STR))
    {
	int port;
	smpd_context_t *rc_context;

	smpd_dbg_printf("closing the old socket in the %s context.\n", smpd_get_context_str(context));
	/* close the old sock */
	smpd_dbg_printf("SMPDU_Sock_post_close(%d)\n", SMPDU_Sock_get_sock_id(context->sock));
	/*context->state = SMPD_CLOSING;*/ /* don't set it here, set it later after the state has been copied to the new context */
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("SMPDU_Sock_post_close failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	smpd_dbg_printf("connecting a new socket.\n");
	/* reconnect */
	port = atol(context->port_str);
	if (port < 1)
	{
	    smpd_err_printf("Invalid reconnect port read: %d\n", port);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_create_context(context->type, context->set, SMPDU_SOCK_INVALID_SOCK, context->id, &rc_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a new context for the reconnection.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	rc_context->state = context->state;
	rc_context->write_state = SMPD_RECONNECTING;
	context->state = SMPD_CLOSING;
	rc_context->connect_to = context->connect_to;
	rc_context->connect_return_id = context->connect_return_id;
	rc_context->connect_return_tag = context->connect_return_tag;
	strcpy(rc_context->host, context->host);
	/* replace the old context with the new */
	if (rc_context->type == SMPD_CONTEXT_LEFT_CHILD || rc_context->type == SMPD_CONTEXT_CHILD)
	    smpd_process.left_context = rc_context;
	if (rc_context->type == SMPD_CONTEXT_RIGHT_CHILD)
	    smpd_process.right_context = rc_context;
	smpd_dbg_printf("posting a re-connect to %s:%d in %s context.\n", rc_context->connect_to->host, port, smpd_get_context_str(rc_context));
	result = SMPDU_Sock_post_connect(rc_context->set, rc_context, rc_context->connect_to->host, port, &rc_context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to post a connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result));
	    if (smpd_post_abort_command("Unable to connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result)) != SMPD_SUCCESS)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    result = smpd_generate_session_header(context->session_header, context->connect_to->id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = SMPDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_session_header"
int smpd_state_reading_session_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the session header, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read session header: '%s'\n", context->session_header);
    result = smpd_interpret_session_header(context->session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to interpret the session header: '%s'\n", context->session_header);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->type = SMPD_CONTEXT_PARENT;
    context->id = smpd_process.parent_id; /* set by smpd_interpret_session_header */
    if (smpd_process.parent_context && smpd_process.parent_context != context)
	smpd_err_printf("replacing parent context.\n");
    smpd_process.parent_context = context;
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command\n");
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = SMPDU_Sock_post_close(context->sock);
	    smpd_exit_fn(FCNAME);
	    return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_session_header"
int smpd_state_writing_session_header(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_context_t *result_context, *context_in;
    SMPDU_SOCK_NATIVE_FD stdin_fd;
    SMPDU_Sock_t insock;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif
#ifdef USE_PTHREAD_STDIN_REDIRECTION
    int fd[2];
#endif
    smpd_host_node_t *left, *right, *host_node;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the session header, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session header: '%s'\n", context->session_header);
    switch (context->state)
    {
    case SMPD_MPIEXEC_CONNECTING_TREE:
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* mark the node as connected */
	context->connect_to->connected = SMPD_TRUE;
	left = context->connect_to->left;
	right = context->connect_to->right;

	while (left != NULL || right != NULL)
	{
	    if (left != NULL)
	    {
		smpd_dbg_printf("creating connect command for left node\n");
		host_node = left;
		left = NULL;
	    }
	    else
	    {
		smpd_dbg_printf("creating connect command for right node\n");
		host_node = right;
		right = NULL;
	    }
	    smpd_dbg_printf("creating connect command to '%s'\n", host_node->host);
	    /* create a connect command to be sent to the parent */
	    result = smpd_create_command("connect", 0, host_node->parent, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a connect command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    host_node->connect_cmd_tag = cmd_ptr->tag;
	    result = smpd_add_command_arg(cmd_ptr, "host", host_node->host);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", host_node->host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "id", host_node->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", host_node->host);
		smpd_exit_fn(FCNAME);
		return result;
	    }
	    if (smpd_process.plaintext)
	    {
		/* propagate the plaintext option to the manager doing the connect */
		result = smpd_add_command_arg(cmd_ptr, "plaintext", "yes");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the plaintext parameter to the connect command for host %s\n", host_node->host);
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the connect command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}

	host_node = smpd_process.host_list;
	while (host_node != NULL)
	{
	    if (host_node->connected == SMPD_FALSE)
	    {
		smpd_dbg_printf("not connected yet: %s not connected\n", host_node->host);
		break;
	    }
	    host_node = host_node->next;
	}
	if (host_node == NULL)
	{
	    /* Everyone connected, send the start_dbs command */
	    /* create the start_dbs command to be sent to the first host */
	    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a start_dbs command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }

	    if (context->spawn_context)
	    {
		smpd_dbg_printf("spawn_context found, adding preput values to the start_dbs command.\n");
		result = smpd_add_command_int_arg(cmd_ptr, "npreput", context->spawn_context->npreput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}

		result = smpd_add_command_arg(cmd_ptr, "preput", context->spawn_context->preput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn(FCNAME);
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the start_dbs command.\n");
		smpd_exit_fn(FCNAME);
		return result;
	    }
	}
	break;
    case SMPD_CONNECTING:
	/* post a read of the next command on the newly connected context */
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/* send the successful result command back to the connector */
	result = smpd_create_command("result", smpd_process.id, context->connect_return_id, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "cmd_tag", context->connect_return_tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "cmd_orig", "connect");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add cmd_orig to the result command for a connect command\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_command_destination(context->connect_return_id, &result_context);
	smpd_dbg_printf("sending result command: \"%s\"\n", cmd_ptr->cmd);
	result = smpd_post_write_command(result_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(result_context));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	break;
    case SMPD_MPIEXEC_CONNECTING_SMPD:
	/* post a read for a possible incoming command */
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
		context->host, get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}

	/* check to see if this is a shutdown session */
	if (smpd_process.builtin_cmd == SMPD_CMD_SHUTDOWN)
	{
	    result = smpd_create_command("shutdown", 0, 1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a shutdown command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the shutdown command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a restart session */
	if (smpd_process.builtin_cmd == SMPD_CMD_RESTART)
	{
	    result = smpd_create_command("restart", 0, 1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a restart command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the restart command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a validate session */
	if (smpd_process.builtin_cmd == SMPD_CMD_VALIDATE)
	{
	    result = smpd_create_command("validate", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a validate command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account(%s) to the validate command.\n", smpd_process.UserAccount);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_encrypt_data(smpd_process.UserPassword, (int)strlen(smpd_process.UserPassword)+1, context->encrypted_password, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to encrypt the password for the validate command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "password", context->encrypted_password);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password to the validate command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the validate command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a set session */
	if (smpd_process.builtin_cmd == SMPD_CMD_SET)
	{
	    result = smpd_create_command("set", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a set command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "key", smpd_process.key);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the key(%s) to the set command.\n", smpd_process.key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "value", smpd_process.val);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the value(%s) to the set command.\n", smpd_process.val);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the set command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a status session */
	if (smpd_process.builtin_cmd == SMPD_CMD_DO_STATUS)
	{
	    result = smpd_create_command("status", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a status command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the status command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is an add_job session */
	if (smpd_process.builtin_cmd == SMPD_CMD_ADD_JOB)
	{
	    result = smpd_create_command("add_job", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create an add_job command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "key", smpd_process.job_key); /*context->sspi_job_key ??? */
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job key(%s) to the add_job command.\n", smpd_process.job_key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "username", smpd_process.job_key_account);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job key(%s) to the add_job command.\n", smpd_process.job_key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the add_job command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is an add_job_and_password session */
	if (smpd_process.builtin_cmd == SMPD_CMD_ADD_JOB_AND_PASSWORD)
	{
	    char buffer[SMPD_MAX_PASSWORD_LENGTH];
	    result = smpd_create_command("add_job_and_password", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create an add_job_and_password command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "key", smpd_process.job_key); /*context->sspi_job_key ??? */
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job key(%s) to the add_job_and_password command.\n", smpd_process.job_key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "username", smpd_process.job_key_account);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job account(%s) to the add_job_and_password command.\n", smpd_process.job_key_account);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_encrypt_data(smpd_process.job_key_password, (int)strlen(smpd_process.job_key_password), buffer, SMPD_MAX_PASSWORD_LENGTH);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to encrypt the job password.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "password", buffer);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job password to the add_job_and_password command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the add_job_and_password command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is an remove_job session */
	if (smpd_process.builtin_cmd == SMPD_CMD_REMOVE_JOB)
	{
	    result = smpd_create_command("remove_job", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a remove_job command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "key", smpd_process.job_key); /*context->sspi_job_key ??? */
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job key(%s) to the remove_job command.\n", smpd_process.job_key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the remove_job command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is an associate_job session */
	if (smpd_process.builtin_cmd == SMPD_CMD_ASSOCIATE_JOB)
	{
	    result = smpd_create_command("associate_job", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create an associate_job command.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "key", smpd_process.job_key); /*context->sspi_job_key ??? */
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the job key(%s) to the associate_job command.\n", smpd_process.job_key);
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the associate_job command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	}

	/* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
	result = smpd_make_socket_loop((SOCKET*)&stdin_fd, &hWrite);
	if (result)
	{
	    smpd_err_printf("Unable to make a local socket loop to forward stdin.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
	socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
	smpd_process.stdin_read = fd[0];
	smpd_process.stdin_write = fd[1];
	stdin_fd = fd[0];
#else
	stdin_fd = fileno(stdin);
#endif

	/* convert the native handle to a sock */
	/*printf("stdin native sock %d\n", stdin_fd);fflush(stdout);*/
	result = SMPDU_Sock_native_to_sock(context->set, stdin_fd, NULL, &insock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a sock from stdin,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/* create a context for reading from stdin */
	result = smpd_create_context(SMPD_CONTEXT_STDIN, set, insock, -1, &context_in);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a context for stdin.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	SMPDU_Sock_set_user_ptr(insock, context_in);

#ifdef HAVE_WINDOWS_H
	/* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
	stdin to a sock */
	smpd_process.hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (smpd_process.hCloseStdinThreadEvent == NULL)
	{
	    smpd_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_process.hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_stdin_thread, (void*)hWrite, 0, &dwThreadID);
	if (smpd_process.hStdinThread == NULL)
	{
	    smpd_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
	if (pthread_create(&smpd_process.stdin_thread, NULL, smpd_pthread_stdin_thread, NULL) != 0)
	{
	    smpd_err_printf("Unable to create a thread to read stdin, error %d\n", errno);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/*pthread_detach(smpd_process.stdin_thread);*/
#endif

	/* post a read for a user command from stdin */
	context_in->read_state = SMPD_READING_STDIN;
	result = SMPDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	break;
    default:
	smpd_err_printf("wrote session header while in state %d\n", context->state);
	break;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_smpd_result"
int smpd_state_reading_smpd_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the smpd result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read smpd result: '%s'\n", context->pwd_request);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	smpd_dbg_printf("connection rejected, server returned - %s\n", context->pwd_request);
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to close sock, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/* abort here? */
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    result = smpd_generate_session_header(context->session_header, 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = SMPDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_process_result"
int smpd_state_reading_process_result(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the process session result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read process session result: '%s'\n", context->pwd_request);
    if (strcmp(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	char *host_ptr;
#ifdef HAVE_WINDOWS_H
	smpd_delete_cached_password();
#endif
	if (smpd_process.do_console && smpd_process.console_host[0] != '\0')
	    host_ptr = smpd_process.console_host;
	else if (context->connect_to && context->connect_to->host[0] != '\0')
	    host_ptr = context->connect_to->host;
	else if (context->host[0] != '\0')
	    host_ptr = context->host;
	else
	    host_ptr = NULL;
	if (host_ptr)
	    printf("Credentials for %s rejected connecting to %s\n", context->account, host_ptr);
	else
	    printf("Credentials for %s rejected.\n", context->account);
	fflush(stdout);
	smpd_dbg_printf("process session rejected\n");
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to close sock,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	/* when does a forming context get assinged it's global place?  At creation?  At connection? */
	smpd_process.left_context = NULL;
	if (host_ptr)
	{
	    result = smpd_post_abort_command("Unable to connect to %s", host_ptr);
	}
	else
	{
	    result = smpd_post_abort_command("connection failed");
	}
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    context->read_state = SMPD_READING_RECONNECT_REQUEST;
    result = SMPDU_Sock_post_read(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the re-connect request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_session_accept"
int smpd_state_writing_session_accept(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the session accept string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session accept: '%s'\n", context->pwd_request);
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = SMPDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_session_reject"
int smpd_state_writing_session_reject(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the session reject string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session reject: '%s'\n", context->pwd_request);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = SMPDU_Sock_post_close(context->sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a close of the sock after writing a reject message,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_process_session_accept"
int smpd_state_writing_process_session_accept(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the process session accept string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session accept: '%s'\n", context->pwd_request);
    /* post a write of the reconnect request */
    smpd_dbg_printf("smpd writing reconnect request: port %s\n", context->port_str);
    context->write_state = SMPD_WRITING_RECONNECT_REQUEST;
    result = SMPDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_process_session_reject"
int smpd_state_writing_process_session_reject(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the process session reject string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session reject: '%s'\n", context->pwd_request);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = SMPDU_Sock_post_close(context->sock);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a close of the sock after writing a process session reject message,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_smpd_password"
int smpd_state_writing_smpd_password(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the smpd password, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote smpd password.\n");
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SMPD_RESULT;
    result = SMPDU_Sock_post_read(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session request result,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_pmi_id"
int smpd_state_reading_pmi_id(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the pmi context id, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = SMPDU_Sock_post_close(context->sock);
	smpd_exit_fn(FCNAME);
	return (result == SMPD_SUCCESS) ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read pmi context id: '%s'\n", context->session);
    smpd_exit_fn(FCNAME);
    return SMPD_DBS_RETURN;
}

#undef FCNAME
#define FCNAME "smpd_state_writing_pmi_id"
int smpd_state_writing_pmi_id(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to write the pmi context id, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote pmi context id: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;

    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of a command after accepting a pmi connection.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_timeout"
int smpd_state_reading_timeout(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(context);

    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the timeout byte, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_kill_all_processes();
#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
    {
	SetEvent(smpd_process.hCloseStdinThreadEvent);
    }
    if (smpd_process.hStdinThread != NULL)
    {
	/* close stdin so the input thread will exit */
	CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
	if (WaitForSingleObject(smpd_process.hStdinThread, 3000) != WAIT_OBJECT_0)
	{
	    TerminateThread(smpd_process.hStdinThread, 321);
	}
	CloseHandle(smpd_process.hStdinThread);
    }
    /*
    if (smpd_process.hCloseStdinThreadEvent)
    {
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
	smpd_process.hCloseStdinThreadEvent = NULL;
    }
    */
#elif defined(USE_PTHREAD_STDIN_REDIRECTION)
    smpd_cancel_stdin_thread();
#endif
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_reading_mpiexec_abort"
int smpd_state_reading_mpiexec_abort(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(context);

    if (event_ptr->error != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the mpiexec abort byte, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (smpd_process.pg_list == NULL)
    {
	/* no processes have been started yet, so exit here */
	smpd_exit(-1);
    }
    result = smpd_abort_job(smpd_process.pg_list->kvs, 0, "mpiexec aborting job");

    smpd_exit_fn(FCNAME);
    /* return failure here and the mpiexec_abort context will be closed */
    /*return SMPD_FAIL;*/
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_singleton_mpiexec_connecting" 
int smpd_state_singleton_mpiexec_connecting(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr){
    int result;
    smpd_command_t *cmd_ptr=NULL;
    smpd_enter_fn(FCNAME);
    result = smpd_create_command("singinit_info", smpd_process.id, 1, SMPD_FALSE, &cmd_ptr);
    if(result == SMPD_SUCCESS){
        result = smpd_add_command_arg(cmd_ptr, "kvsname", context->singleton_init_kvsname);
    }
    if(result == SMPD_SUCCESS){
        result = smpd_add_command_arg(cmd_ptr, "domainname", context->singleton_init_domainname);
    }
    if(result == SMPD_SUCCESS){
        result = smpd_add_command_arg(cmd_ptr, "host", context->singleton_init_hostname);
    }
    if(result == SMPD_SUCCESS){
        result = smpd_add_command_int_arg(cmd_ptr, "port", context->singleton_init_pm_port);
    }
    if(result == SMPD_SUCCESS){
        result = smpd_post_write_command(context, cmd_ptr);
    }
    context->state = SMPD_SINGLETON_DONE;

    if(result != SMPD_SUCCESS){
        context->state = SMPD_DONE;
        smpd_err_printf("smpd_create_command/smpd_add_command failed\n");
        if(cmd_ptr != NULL){
            smpd_free_command(cmd_ptr);
        }
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_state_singleton_client_listening"
int smpd_state_singleton_client_listening(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set){
    int result;
    SMPDU_Sock_t new_sock;
    smpd_context_t *p_singleton_connected_context;

    smpd_enter_fn(FCNAME);
    /* Accept connection */
    result = SMPDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if(result != SMPD_SUCCESS){
        context->state = SMPD_DONE;
        smpd_err_printf("SMPDU_Sock_accept failed, error = %s\n", get_sock_error_string(result));
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    context->state = SMPD_SINGLETON_DONE;

    /* Close the listen sock and free its context */
    result = SMPDU_Sock_post_close(context->sock);
    if(result != SMPD_SUCCESS){
        context->state = SMPD_DONE;
        smpd_err_printf("SMPDU_Sock_post_close failed, error = %s\n", get_sock_error_string(result));
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    /* Create new context for the accepted connection - btw mpiexec & singleton init client */
    result = smpd_create_context(SMPD_CONTEXT_SINGLETON_INIT_CLIENT, set, new_sock, -1, 
                                    &p_singleton_connected_context);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("smpd_create_context failed, error = %d\n", result);
        context->state = SMPD_DONE;
        /*
        result = SMPDU_Sock_post_close(new_sock);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("SMPDU_Sock_post_close failed, error = %s\n", get_sock_error_string(result));
        }
        */
        /* Let state machine handle close and destroy the context */
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    result = SMPDU_Sock_set_user_ptr(new_sock, p_singleton_connected_context);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("SMPDU_Sock_set_user_ptr failed, error = %d\n", result);
        p_singleton_connected_context->state = SMPD_DONE;
        context->state = SMPD_DONE;
        /* Let state machine handle close and destroy the context */
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }
    
    /* Wait for cmd containing the values */
    p_singleton_connected_context->state = context->state;

	result = smpd_post_read_command(p_singleton_connected_context);
    if(result != SMPD_SUCCESS){
        smpd_err_printf("smpd_post_read_command failed, error = %d\n", result);
        context->state = SMPD_DONE;
        result = SMPDU_Sock_post_close(p_singleton_connected_context->sock);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("SMPDU_Sock_post_close failed, error = %s\n", get_sock_error_string(result));
        }
        smpd_exit_fn(FCNAME);
        return SMPD_FAIL;
    }

    /* The current context will be cleanedup by the state machine -- Close is posted for the listening sock*/
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_handle_op_read"
int smpd_handle_op_read(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn(FCNAME);

    switch (context->read_state)
    {
    case SMPD_READING_CHALLENGE_STRING:
	result = smpd_state_reading_challenge_string(context, event_ptr);
	break;
    case SMPD_READING_CONNECT_RESULT:
	result = smpd_state_reading_connect_result(context, event_ptr);
	break;
    case SMPD_READING_SMPD_RESULT:
	result = smpd_state_reading_smpd_result(context, event_ptr);
	break;
    case SMPD_READING_PROCESS_RESULT:
	result = smpd_state_reading_process_result(context, event_ptr);
	break;
    case SMPD_READING_CHALLENGE_RESPONSE:
	result = smpd_state_reading_challenge_response(context, event_ptr);
	break;
    case SMPD_READING_STDIN:
	result = smpd_state_reading_stdin(context, event_ptr);
	break;
    case SMPD_READING_STDOUT:
    case SMPD_READING_STDERR:
	result = smpd_state_reading_stdouterr(context, event_ptr);
	break;
    case SMPD_READING_CMD_HEADER:
	result = smpd_state_reading_cmd_header(context, event_ptr);
	break;
    case SMPD_READING_CMD:
	result = smpd_state_reading_cmd(context, event_ptr);
	break;
    case SMPD_READING_SESSION_REQUEST:
	result = smpd_state_reading_session_request(context, event_ptr);
	break;
    case SMPD_READING_PWD_REQUEST:
	result = smpd_state_reading_pwd_request(context, event_ptr);
	break;
    case SMPD_READING_SMPD_PASSWORD:
	result = smpd_state_reading_smpd_password(context, event_ptr);
	break;
    case SMPD_READING_CRED_ACK:
	result = smpd_state_reading_cred_ack(context, event_ptr);
	break;
    case SMPD_READING_ACCOUNT:
	result = smpd_state_reading_account(context, event_ptr);
	break;
    case SMPD_READING_PASSWORD:
	result = smpd_state_reading_password(context, event_ptr);
	break;
    case SMPD_READING_CRED_REQUEST:
	result = smpd_state_reading_cred_request(context, event_ptr);
	break;
    case SMPD_READING_RECONNECT_REQUEST:
	result = smpd_state_reading_reconnect_request(context, event_ptr);
	break;
    case SMPD_READING_SESSION_HEADER:
	result = smpd_state_reading_session_header(context, event_ptr);
	break;
    case SMPD_READING_TIMEOUT:
	result = smpd_state_reading_timeout(context, event_ptr);
	break;
    case SMPD_READING_MPIEXEC_ABORT:
	result = smpd_state_reading_mpiexec_abort(context, event_ptr);
	break;
    case SMPD_READING_PMI_ID:
	result = smpd_state_reading_pmi_id(context, event_ptr);
	break;
    case SMPD_READING_SSPI_HEADER:
	result = smpd_state_reading_sspi_header(context, event_ptr);
	break;
    case SMPD_READING_SSPI_BUFFER:
	result = smpd_state_reading_sspi_buffer(context, event_ptr);
	break;
    case SMPD_READING_DELEGATE_REQUEST_RESULT:
	result = smpd_state_reading_delegate_request_result(context, event_ptr);
	break;
    case SMPD_READING_CLIENT_SSPI_HEADER:
	result = smpd_state_reading_client_sspi_header(context, event_ptr);
	break;
    case SMPD_READING_CLIENT_SSPI_BUFFER:
	result = smpd_state_reading_client_sspi_buffer(context, event_ptr);
	break;
    case SMPD_READING_IMPERSONATE_RESULT:
	result = smpd_state_reading_impersonate_result(context, event_ptr);
	break;
    case SMPD_READING_SSPI_JOB_KEY:
	result = smpd_state_reading_sspi_job_key(context, event_ptr);
	break;
    default:
	smpd_err_printf("sock_op_read returned while context is in state: %s\n",
	    smpd_get_state_string(context->read_state));
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_op_write"
int smpd_handle_op_write(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;

    smpd_enter_fn(FCNAME);
    switch (context->write_state)
    {
    case SMPD_WRITING_CHALLENGE_RESPONSE:
	result = smpd_state_writing_challenge_response(context, event_ptr);
	break;
    case SMPD_WRITING_CHALLENGE_STRING:
	result = smpd_state_writing_challenge_string(context, event_ptr);
	break;
    case SMPD_WRITING_CONNECT_RESULT:
	result = smpd_state_writing_connect_result(context, event_ptr);
	break;
    case SMPD_WRITING_CMD:
	result = smpd_state_writing_cmd(context, event_ptr);
	break;
    case SMPD_WRITING_SMPD_SESSION_REQUEST:
	result = smpd_state_writing_smpd_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_REQUEST:
	result = smpd_state_writing_process_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PMI_SESSION_REQUEST:
	result = smpd_state_writing_pmi_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PWD_REQUEST:
	result = smpd_state_writing_pwd_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_PWD_REQUEST:
	result = smpd_state_writing_no_pwd_request(context, event_ptr);
	break;
    case SMPD_WRITING_SSPI_REQUEST:
	result = smpd_state_writing_sspi_request(context, event_ptr);
	break;
    case SMPD_WRITING_SMPD_PASSWORD:
	result = smpd_state_writing_smpd_password(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_REQUEST:
	result = smpd_state_writing_cred_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_CRED_REQUEST:
	result = smpd_state_writing_no_cred_request(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_YES:
	result = smpd_state_writing_cred_ack_yes(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_NO:
	result = smpd_state_writing_cred_ack_no(context, event_ptr);
	break;
    case SMPD_WRITING_RECONNECT_REQUEST:
	result = smpd_state_writing_reconnect_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_RECONNECT_REQUEST:
	result = smpd_state_writing_no_reconnect_request(context, event_ptr);
	break;
    case SMPD_WRITING_ACCOUNT:
	result = smpd_state_writing_account(context, event_ptr);
	break;
    case SMPD_WRITING_PASSWORD:
	result = smpd_state_writing_password(context, event_ptr);
	break;
    case SMPD_WRITING_SESSION_HEADER:
	result = smpd_state_writing_session_header(context, event_ptr, set);
	break;
    case SMPD_WRITING_SESSION_ACCEPT:
	result = smpd_state_writing_session_accept(context, event_ptr);
	break;
    case SMPD_WRITING_SESSION_REJECT:
	result = smpd_state_writing_session_reject(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_ACCEPT:
	result = smpd_state_writing_process_session_accept(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_REJECT:
	result = smpd_state_writing_process_session_reject(context, event_ptr);
	break;
    case SMPD_WRITING_DATA_TO_STDIN:
	result = smpd_state_smpd_writing_data_to_stdin(context, event_ptr);
	break;
    case SMPD_WRITING_PMI_ID:
	result = smpd_state_writing_pmi_id(context, event_ptr);
	break;
    case SMPD_WRITING_SSPI_HEADER:
	result = smpd_state_writing_sspi_header(context, event_ptr);
	break;
    case SMPD_WRITING_SSPI_BUFFER:
	result = smpd_state_writing_sspi_buffer(context, event_ptr);
	break;
    case SMPD_WRITING_DELEGATE_REQUEST:
	result = smpd_state_writing_delegate_request(context, event_ptr);
	break;
    case SMPD_WRITING_IMPERSONATE_RESULT:
	result = smpd_state_writing_impersonate_result(context, event_ptr);
	break;
    case SMPD_WRITING_CLIENT_SSPI_HEADER:
	result = smpd_state_writing_client_sspi_header(context, event_ptr);
	break;
    case SMPD_WRITING_CLIENT_SSPI_BUFFER:
	result = smpd_state_writing_client_sspi_buffer(context, event_ptr);
	break;
    case SMPD_WRITING_DELEGATE_REQUEST_RESULT:
	result = smpd_state_writing_delegate_request_result(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_SSPI:
	result = smpd_state_writing_cred_ack_sspi(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_SSPI_JOB_KEY:
	result = smpd_state_writing_cred_ack_sspi_job_key(context, event_ptr);
	break;
    case SMPD_WRITING_SSPI_JOB_KEY:
	result = smpd_state_writing_sspi_job_key(context, event_ptr);
	break;
    default:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("sock_op_write failed while context is in state %s, %s\n",
		smpd_get_state_string(context->write_state), get_sock_error_string(event_ptr->error));
	}
	else
	{
	    smpd_err_printf("sock_op_write returned while context is in state %s, %s\n",
		smpd_get_state_string(context->write_state), get_sock_error_string(event_ptr->error));
	}
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_op_accept"
int smpd_handle_op_accept(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr, SMPDU_Sock_set_t set)
{
    int result;

    smpd_enter_fn(FCNAME);
    switch (context->state)
    {
    case SMPD_SMPD_LISTENING:
	result = smpd_state_smpd_listening(context, event_ptr, set);
	break;
    case SMPD_MGR_LISTENING:
	result = smpd_state_mgr_listening(context, event_ptr, set);
	break;
    case SMPD_PMI_LISTENING:
	result = smpd_state_pmi_listening(context, event_ptr, set);
	break;
    case SMPD_PMI_SERVER_LISTENING:
	result = smpd_state_pmi_server_listening(context, event_ptr, set);
	break;
    case SMPD_SINGLETON_CLIENT_LISTENING:
    result = smpd_state_singleton_client_listening(context, event_ptr, set);
    break;
    default:
	smpd_err_printf("sock_op_accept returned while context is in state: %s\n",
	    smpd_get_state_string(context->state));
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_op_connect"
int smpd_handle_op_connect(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn(FCNAME);
    switch (context->state)
    {
    case SMPD_MPIEXEC_CONNECTING_TREE:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect mpiexec tree, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
    case SMPD_CONNECTING:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect an internal tree node, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
	if (context->write_state == SMPD_RECONNECTING)
	{
	    result = smpd_generate_session_header(context->session_header, context->connect_to->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to generate a session header.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    context->write_state = SMPD_WRITING_SESSION_HEADER;
	    result = SMPDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = SMPD_SUCCESS;
	    break;
	}
	/* no break here, not-reconnecting contexts fall through */
    case SMPD_MPIEXEC_CONNECTING_SMPD:
    case SMPD_CONNECTING_RPMI:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect to the smpd, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
	smpd_dbg_printf("connect succeeded, posting read of the challenge string\n");
	context->read_state = SMPD_READING_CHALLENGE_STRING;
	result = SMPDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the challenge string,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	result = SMPD_SUCCESS;
	break;
    case SMPD_CONNECTING_PMI:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect to the smpd, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
	/* 
	smpd_dbg_printf("connect succeeded, posting read of the next pmi command\n");
	context->read_state = SMPD_READING_CMD_HEADER;
	result = SMPDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the challenge string,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	*/
	context->write_state = SMPD_IDLE;
	result = SMPD_DBS_RETURN;
	break;
    case SMPD_SINGLETON_MPIEXEC_CONNECTING:
        result = smpd_state_singleton_mpiexec_connecting(context, event_ptr);
        if(result != SMPD_SUCCESS){
            smpd_err_printf("smpd_state_singleton_mpiexec_connecting failed, error = %d\n", result);
            result = SMPD_FAIL;
        }
    break;
    default:
	if (event_ptr->error != SMPD_SUCCESS)
	{
	    smpd_err_printf("sock_op_connect failed while %s context is in state %s, %s\n",
		smpd_get_context_str(context), smpd_get_state_string(context->state),
		get_sock_error_string(event_ptr->error));
	}
	else
	{
	    smpd_err_printf("sock_op_connect returned while %s context is in state %s, %s\n",
		smpd_get_context_str(context), smpd_get_state_string(context->state),
		get_sock_error_string(event_ptr->error));
	}
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn(FCNAME);
    return result;
}

#undef FCNAME
#define FCNAME "smpd_handle_op_close"
int smpd_handle_op_close(smpd_context_t *context, SMPDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(event_ptr);

    smpd_dbg_printf("op_close received - %s state.\n", smpd_get_state_string(context->state));
    switch (context->state)
    {
    case SMPD_SMPD_LISTENING:
    case SMPD_MGR_LISTENING:
	smpd_process.listener_context = NULL;
	break;
    case SMPD_DONE:
	    smpd_free_context(context);
	    smpd_exit_fn(FCNAME);
	    return SMPD_EXIT;
    case SMPD_SINGLETON_DONE:
        smpd_free_context(context);
        smpd_exit_fn(FCNAME);
        return SMPD_SUCCESS;
    case SMPD_RESTARTING:
	smpd_restart();
	break;
    case SMPD_EXITING:
	if (smpd_process.listener_context)
	{
	    smpd_process.listener_context->state = SMPD_EXITING;
	    smpd_dbg_printf("closing the listener (state = %s).\n", smpd_get_state_string(smpd_process.listener_context->state));
	    result = SMPDU_Sock_post_close(smpd_process.listener_context->sock);
	    smpd_process.listener_context = NULL;
	    if (result == SMPD_SUCCESS)
	    {
		break;
	    }
	    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
		get_sock_error_string(result));
	}
	smpd_free_context(context);
	/*smpd_exit(0);*/
#ifdef HAVE_WINDOWS_H
	if (smpd_process.bService)
	    SetEvent(smpd_process.hBombDiffuseEvent);
#endif
	smpd_exit_fn(FCNAME);
	return SMPD_EXIT;
    case SMPD_CLOSING:
	if (context->process && (context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR || context->type == SMPD_CONTEXT_PMI))
	{
	    context->process->context_refcount--;
	    if (context->process->context_refcount < 1)
	    {
		smpd_process_t *trailer, *iter;

		smpd_dbg_printf("process refcount == %d, waiting for the process to finish exiting.\n", context->process->context_refcount);

		if (context->process->local_process)
		{
#ifdef HAVE_WINDOWS_H
		    smpd_process_from_registry(context->process);
#endif
		    result = smpd_wait_process(context->process->wait, &context->process->exitcode);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to wait for the process to exit: '%s'\n", context->process->exe);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}

		if (context->process->in != NULL)
		{
		    smpd_context_t *in_context = context->process->in;
		    context->process->in = NULL;
		    in_context->state = SMPD_CLOSING;
		    in_context->process = NULL; /* NULL this out so the closing of the stdin context won't access it after it has been freed */
		    result = SMPDU_Sock_post_close(in_context->sock);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_free_context(in_context);
		    }
		}

		if (smpd_process.parent_context != NULL)
		{
		    /* create the process exited command */
		    result = smpd_create_command("exit", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create an exit command for rank %d\n", context->process->rank);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    result = smpd_add_command_int_arg(cmd_ptr, "rank", context->process->rank);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the rank %d to the exit command.\n", context->process->rank);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    result = smpd_add_command_int_arg(cmd_ptr, "code", context->process->exitcode);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the exit code to the exit command for rank %d\n", context->process->rank);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    result = smpd_add_command_arg(cmd_ptr, "kvs", context->process->kvs_name);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add the kvs name to the exit command for rank %d\n", context->process->rank);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		    smpd_dbg_printf("creating an exit command for rank %d, pid %d, exit code %d.\n",
			context->process->rank, context->process->pid, context->process->exitcode);

		    /* send the exit command */
		    result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the exit command for rank %d\n", context->process->rank);
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
		}

		/* remove the process structure from the global list */
		trailer = iter = smpd_process.process_list;
		while (iter)
		{
		    if (context->process == iter)
		    {
			if (iter == smpd_process.process_list)
			{
			    smpd_process.process_list = smpd_process.process_list->next;
			}
			else
			{
			    trailer->next = iter->next;
			}
			/* NULL the context pointer just to be sure no one uses it after it is freed. */
			switch (context->type)
			{
			case SMPD_CONTEXT_STDIN:
			    context->process->in = NULL;
			    break;
			case SMPD_CONTEXT_STDOUT:
			    context->process->out = NULL;
			    break;
			case SMPD_CONTEXT_STDERR:
			    context->process->err = NULL;
			    break;
			case SMPD_CONTEXT_PMI:
			    context->process->pmi = NULL;
			    break;
			}
			/* free the process structure */
			smpd_free_process_struct(iter);
			context->process = NULL;
			iter = NULL;
		    }
		    else
		    {
			if (trailer != iter)
			    trailer = trailer->next;
			iter = iter->next;
		    }
		}
	    }
	    else
	    {
		smpd_dbg_printf("process refcount == %d, %s closed.\n", context->process->context_refcount, smpd_get_context_str(context));
		/* NULL the context pointer just to be sure no one uses it after it is freed. */
		switch (context->type)
		{
		case SMPD_CONTEXT_STDIN:
		    context->process->in = NULL;
		    break;
		case SMPD_CONTEXT_STDOUT:
		    context->process->out = NULL;
		    break;
		case SMPD_CONTEXT_STDERR:
		    context->process->err = NULL;
		    break;
		case SMPD_CONTEXT_PMI:
		    context->process->pmi = NULL;
		    break;
		}
	    }
	}
	else if (context->process && (context->type == SMPD_CONTEXT_STDOUT_RSH || context->type == SMPD_CONTEXT_STDERR_RSH))
	{
	    context->process->context_refcount--;
	    if (context->process->context_refcount < 1)
	    {
		smpd_process_t *trailer, *iter;

		smpd_dbg_printf("process refcount == %d, waiting for the process to finish exiting.\n", context->process->context_refcount);
		/*printf("process refcount == %d, waiting for the process to finish exiting.\n", context->process->context_refcount);fflush(stdout);*/

#ifdef HAVE_WINDOWS_H
		smpd_process_from_registry(context->process);
#endif
		result = smpd_wait_process(context->process->wait, &context->process->exitcode);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to wait for the process to exit: '%s'\n", context->process->exe);
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}

		/* remove the process structure from the global list */
		trailer = iter = smpd_process.process_list;
		while (iter)
		{
		    if (context->process == iter)
		    {
			if (iter == smpd_process.process_list)
			{
			    smpd_process.process_list = smpd_process.process_list->next;
			}
			else
			{
			    trailer->next = iter->next;
			}
			/* NULL the context pointer just to be sure no one uses it after it is freed. */
			switch (context->type)
			{
			case SMPD_CONTEXT_STDIN:
			    context->process->in = NULL;
			    break;
			case SMPD_CONTEXT_STDOUT_RSH:
			    context->process->out = NULL;
			    break;
			case SMPD_CONTEXT_STDERR_RSH:
			    context->process->err = NULL;
			    break;
			case SMPD_CONTEXT_PMI:
			    context->process->pmi = NULL;
			    break;
			}
			/* free the process structure */
			smpd_free_process_struct(iter);
			context->process = NULL;
			iter = NULL;
		    }
		    else
		    {
			if (trailer != iter)
			    trailer = trailer->next;
			iter = iter->next;
		    }
		}

		smpd_process.nproc_exited++;
		/*printf("nproc_exited = %d\n", smpd_process.nproc_exited);fflush(stdout);*/
		if (smpd_process.nproc_exited >= smpd_process.nproc)
		{
		    /*smpd_free_context(context);*/
		    smpd_dbg_printf("process exited and all contexts closed, exiting state machine.\n");
		    /*printf("process exited and all contexts closed, exiting state machine.\n");fflush(stdout);*/
		    smpd_exit_fn(FCNAME);
		    return SMPD_EXIT;
		}
	    }
	    else
	    {
		smpd_dbg_printf("process refcount == %d, %s closed.\n", context->process->context_refcount, smpd_get_context_str(context));
		/* NULL the context pointer just to be sure no one uses it after it is freed. */
		switch (context->type)
		{
		case SMPD_CONTEXT_STDIN:
		    context->process->in = NULL;
		    break;
		case SMPD_CONTEXT_STDOUT_RSH:
		    context->process->out = NULL;
		    break;
		case SMPD_CONTEXT_STDERR_RSH:
		    context->process->err = NULL;
		    break;
		case SMPD_CONTEXT_PMI:
		    context->process->pmi = NULL;
		    break;
		}
	    }
	    break;
	}
	else
	{
	    smpd_dbg_printf("Unaffiliated %s context closing.\n", smpd_get_context_str(context));
	}

	if (context == smpd_process.left_context)
	    smpd_process.left_context = NULL;
	if (context == smpd_process.right_context)
	    smpd_process.right_context = NULL;
	if (context == smpd_process.parent_context)
	    smpd_process.parent_context = NULL;
	if (context == smpd_process.listener_context)
	    smpd_process.listener_context = NULL;
	if (smpd_process.closing && smpd_process.left_context == NULL && smpd_process.right_context == NULL)
	{
	    if (smpd_process.parent_context == NULL)
	    {
		if (smpd_process.listener_context)
		{
		    smpd_dbg_printf("all contexts closed, closing the listener.\n");
		    smpd_process.listener_context->state = SMPD_EXITING;
		    result = SMPDU_Sock_post_close(smpd_process.listener_context->sock);
		    if (result == SMPD_SUCCESS)
		    {
			break;
		    }
		    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
			get_sock_error_string(result));
		}
		smpd_free_context(context);
		smpd_dbg_printf("all contexts closed, exiting state machine.\n");
		/*smpd_exit(0);*/
		smpd_exit_fn(FCNAME);
		return SMPD_EXIT;
	    }
	    else
	    {
		/* both children are closed, send closed_request to parent */
		result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a closed_request command for the parent context.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		/*smpd_dbg_printf("posting write of closed_request command to parent: \"%s\"\n", cmd_ptr->cmd);*/
		result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	}
	else
	{
	    if (smpd_process.root_smpd == SMPD_FALSE && smpd_process.parent_context == NULL && smpd_process.left_context == NULL && smpd_process.right_context == NULL)
	    {
		smpd_context_t *iter = smpd_process.context_list;
		/* If there is no connection to the parent, left child, and right child then exit.
		 * But first check to see that no context is in the process of being formed that would become a left or right child.
		 */
		while (iter)
		{
		    /*if (iter->type == SMPD_CONTEXT_PARENT || iter->type == SMPD_CONTEXT_LEFT_CHILD || iter->type == SMPD_CONTEXT_RIGHT_CHILD)*/
		    if (iter->type != SMPD_CONTEXT_LISTENER)
			break;
		    iter = iter->next;
		}
		if (iter == NULL)
		{
		    if (smpd_process.listener_context)
		    {
			smpd_dbg_printf("all contexts closed, closing the listener.\n");
			smpd_process.listener_context->state = SMPD_EXITING;
			result = SMPDU_Sock_post_close(smpd_process.listener_context->sock);
			if (result == SMPD_SUCCESS)
			{
			    break;
			}
			smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
			    get_sock_error_string(result));
		    }
		    smpd_free_context(context);
		    smpd_dbg_printf("all contexts closed, exiting state machine.\n");
		    /*smpd_exit(0);*/
		    smpd_exit_fn(FCNAME);
		    return SMPD_EXIT;
		}
	    }
	}
	break;
    default:
	smpd_err_printf("sock_op_close returned while %s context is in state: %s\n",
	    smpd_get_context_str(context), smpd_get_state_string(context->state));
	break;
    }
    /* free the context */
    result = smpd_free_context(context);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_enter_at_state"
int smpd_enter_at_state(SMPDU_Sock_set_t set, smpd_state_t state)
{
    int result;
    SMPDU_Sock_event_t event;
    smpd_context_t *context;
    char error_msg[SMPD_MAX_ERROR_LEN];
    int len;

    smpd_enter_fn(FCNAME);
    SMPD_UNREFERENCED_ARG(state);

    for (;;)
    {
	event.error = SMPD_SUCCESS;
	smpd_dbg_printf("sock_waiting for the next event.\n");
	result = SMPDU_Sock_wait(set, SMPDU_SOCK_INFINITE_TIME, &event);
	if (result != SMPD_SUCCESS)
	{
	    /*
	    if (result == SOCK_ERR_TIMEOUT)
	    {
		smpd_err_printf("Warning: SMPDU_Sock_wait returned SOCK_ERR_TIMEOUT when infinite time was passed in.\n");
		continue;
	    }
	    */
	    if (event.error != SMPD_SUCCESS)
		result = event.error;
	    smpd_err_printf("SMPDU_Sock_wait failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return result;
	}
	context = event.user_ptr;
	if (context == NULL)
	{
	    smpd_err_printf("SMPDU_Sock_wait returned an event with a NULL user pointer.\n");
	    if (event.error != SMPD_SUCCESS)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    continue;
	}
	switch (event.op_type)
	{
	case SMPDU_SOCK_OP_READ:
	    smpd_dbg_printf("SOCK_OP_READ event.error = %d, result = %d, context=%s\n", event.error, result, smpd_get_context_str(context));
	    if (event.error != SMPD_SUCCESS)
        {
		    /* don't print EOF errors because they usually aren't errors */
		    /*if (event.error != SOCK_EOF)*/
		{
		    /* don't print errors from the pmi context because processes that don't 
		       call PMI_Finalize will get read errors that don't need to be printed.
		       */
		    if (context->type != SMPD_CONTEXT_PMI &&
			context->type != SMPD_CONTEXT_STDOUT_RSH &&
			context->type != SMPD_CONTEXT_STDERR_RSH &&
			context->type != SMPD_CONTEXT_MPIEXEC_STDIN_RSH &&
			context->type != SMPD_CONTEXT_MPIEXEC_STDIN)
		    {
			/* FIXME: Reintroduce event.error == SMPDU_SOCK_ERR_CONN_CLOSED once we have a better error code */
            if (!((context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR) /*&& event.error == SMPDU_SOCK_ERR_CONN_CLOSED */))
			{
			    if (!smpd_process.closing)
			    {
				smpd_err_printf("op_read error on %s context: %s\n", smpd_get_context_str(context), get_sock_error_string(event.error));
			    }
			}
		    }
		}
	    }
	    result = smpd_handle_op_read(context, &event);
	    if (result == SMPD_DBS_RETURN)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS || event.error != SMPD_SUCCESS)
	    {
		if (context->type == SMPD_CONTEXT_PARENT && smpd_process.root_smpd != SMPD_TRUE)
		{
		    smpd_err_printf("connection to my parent broken, aborting.\n");
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
		if (context->type == SMPD_CONTEXT_MPIEXEC_STDIN) /* || context->type == SMPD_CONTEXT_MPIEXEC_STDIN_RSH)*/
		{
		    smpd_command_t *cmd_ptr;
		    smpd_dbg_printf("stdin to mpiexec closed.\n");
		    /* FIXME: should we send a message to the root process manager to close stdin to the root process? */
#ifdef HAVE_WINDOWS_H
		    /* If the stdin redirection thread has been signalled to be closed, don't send a close_stdin command */
		    if (WaitForSingleObject(smpd_process.hCloseStdinThreadEvent, 0) != WAIT_OBJECT_0)
		    {
#endif
		    /* create an close_stdin command */
		    result = smpd_create_command("close_stdin", 0, 1 /* input always goes to node 1? */, SMPD_FALSE, &cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create an close_stdin command.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }

		    /* send the stdin command */
		    result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the close_stdin command.\n");
			smpd_exit_fn(FCNAME);
			return SMPD_FAIL;
		    }
#ifdef HAVE_WINDOWS_H
		    }
#endif
		}
		smpd_dbg_printf("SOCK_OP_READ failed - result = %d, closing %s context.\n", result, smpd_get_context_str(context));
		if (result != SMPD_SUCCESS || context->state != SMPD_CLOSING)
		{
		    /* An error occurred and a close was not successfully posted, post one here */
		    context->state = SMPD_CLOSING;
		    result = SMPDU_Sock_post_close(context->sock);
		    if (result != SMPD_SUCCESS)
		    {
			len = SMPD_MAX_ERROR_LEN;
			result = PMPI_Error_string(result, error_msg, &len);
			if (result == SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to post a close on a broken %s context, error: %s\n", smpd_get_context_str(context), error_msg);
			}
			else
			{
			    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
			}
			if (smpd_process.root_smpd != SMPD_TRUE)
			{
			    /* don't let a broken connection cause the root smpd to exit */
			    /* only non-root (manager) nodes return failure */
			    smpd_err_printf("non-root smpd returning SMPD_FAIL do to failed close request after failed read operation.\n");
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
			else
			{
			    smpd_dbg_printf("root smpd ignoring failed close request after failed read operation.\n");
			}
		    }
		}
	    }
	    break;
	case SMPDU_SOCK_OP_WRITE:
	    smpd_dbg_printf("SOCK_OP_WRITE event.error = %d, result = %d, context=%s\n", event.error, result, smpd_get_context_str(context));
	    if (event.error != SMPD_SUCCESS)
	    {
		smpd_err_printf("op_write error on %s context: %s\n", smpd_get_context_str(context), get_sock_error_string(event.error));
	    }
	    result = smpd_handle_op_write(context, &event, set);
	    if (result == SMPD_DBS_RETURN)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS || event.error != SMPD_SUCCESS)
	    {
		smpd_dbg_printf("SOCK_OP_WRITE failed, closing %s context.\n", smpd_get_context_str(context));
		if (context->state != SMPD_CLOSING)
		{
		    context->state = SMPD_CLOSING;
		    result = SMPDU_Sock_post_close(context->sock);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
			if (smpd_process.root_smpd != SMPD_TRUE)
			{
			    /* don't let a broken connection cause the root smpd to exit */
			    /* only non-root (manager) nodes return failure */
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
		    }
		}
	    }
	    break;
	case SMPDU_SOCK_OP_ACCEPT:
	    smpd_dbg_printf("SOCK_OP_ACCEPT event.error = %d, result = %d, context=%s\n", event.error, result, smpd_get_context_str(context));
	    if (event.error != SMPD_SUCCESS)
	    {
		smpd_err_printf("error listening and accepting socket: %s\n", get_sock_error_string(event.error));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    result = smpd_handle_op_accept(context, &event, set);
	    if (result != SMPD_SUCCESS || event.error != SMPD_SUCCESS)
	    {
		if (result != SMPD_SUCCESS)
		    smpd_dbg_printf("SOCK_OP_ACCEPT failed, result = %d, closing %s context.\n", result, smpd_get_context_str(context));
		else
		    smpd_dbg_printf("SOCK_OP_ACCEPT failed, event.error = %d, closing %s context.\n", event.error, smpd_get_context_str(context));
		if (context->state != SMPD_CLOSING)
		{
		    context->state = SMPD_CLOSING;
		    result = SMPDU_Sock_post_close(context->sock);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
			if (smpd_process.root_smpd != SMPD_TRUE)
			{
			    /* don't let a broken connection cause the root smpd to exit */
			    /* only non-root (manager) nodes return failure */
			    smpd_exit_fn(FCNAME);
			    return SMPD_FAIL;
			}
		    }
		}
	    }
	    break;
	case SMPDU_SOCK_OP_CONNECT:
	    smpd_dbg_printf("SOCK_OP_CONNECT event.error = %d, result = %d, context=%s\n", event.error, result, smpd_get_context_str(context));
	    if (event.error != SMPD_SUCCESS)
	    {
		smpd_err_printf("op_connect error: %s\n", get_sock_error_string(event.error));
	    }
	    result = smpd_handle_op_connect(context, &event);
	    if (result == SMPD_DBS_RETURN)
	    {
		smpd_exit_fn(FCNAME);
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS || event.error != SMPD_SUCCESS)
	    {
		smpd_process.state_machine_ret_val = (result != SMPD_SUCCESS) ? result : event.error;
		smpd_dbg_printf("SOCK_OP_CONNECT failed, closing %s context.\n", smpd_get_context_str(context));
		if (context->state != SMPD_CLOSING)
		{
		    context->state = SMPD_CLOSING;
		    result = SMPDU_Sock_post_close(context->sock);
		}
		else
		{
		    result = SMPD_SUCCESS;
		}
		if (context->connect_to)
		    smpd_post_abort_command("unable to connect to %s", context->connect_to->host);
		else
		    smpd_post_abort_command("connect failure");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		}
	    }
	    break;
	case SMPDU_SOCK_OP_CLOSE:
	    smpd_dbg_printf("SOCK_OP_CLOSE event.error = %d, result = %d, context=%s\n", event.error, result, smpd_get_context_str(context));
        fflush(stdout);
	    if (event.error != SMPD_SUCCESS)
		smpd_err_printf("error closing the %s context socket: %s\n", smpd_get_context_str(context), get_sock_error_string(event.error));
	    result = smpd_handle_op_close(context, &event);
	    /*
	    if (event.error != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to close the %s context.\n", smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    */
	    if (result != SMPD_SUCCESS)
	    {
		if (result == SMPD_EXIT)
		{
		    smpd_exit_fn(FCNAME);
		    return SMPD_SUCCESS;
		}
		smpd_err_printf("unable to close the %s context.\n", smpd_get_context_str(context));
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	    break;
	default:
	    smpd_dbg_printf("SOCK_OP_UNKNOWN\n");
	    smpd_err_printf("SMPDU_Sock_wait returned unknown sock event type: %d\n", event.op_type);
	    break;
	}
    }
    /*
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
    */
}

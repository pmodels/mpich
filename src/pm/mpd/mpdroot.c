/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpdroot.h"

#ifdef NEEDS_SNPRINTF_DECL
int snprintf(char *str, size_t size, const char *format, ...);
#endif

int main(int argc, char *argv[])
{
    int rc, sock;
    struct sockaddr_un sa;
    char console_name[NAME_LEN], cmd[NAME_LEN];
    struct passwd *pwent;
    char input_line[NAME_LEN+1], secretword[NAME_LEN+1];
    FILE *conf_file;
    char *mpdconf_path = NULL;

    if ((pwent = getpwuid(getuid())) == NULL)    /* for real id */
    {
        printf("%s: getpwnam failed",argv[0]);
        exit(1);
    }

    /*
     * We look for a readable mpd.conf in the following order.
     * - MPD_CONF_FILE set in environment
     * - .mpd.conf in the user's home directory
     * - The system wide default in SYSCONFDIR/mpd.conf
     */
    mpdconf_path = getenv("MPD_CONF_FILE");
    if ( ! (mpdconf_path && access( mpdconf_path, R_OK ) == 0) ){
        /* By far, the largest we'll need */
        size_t mpdconf_path_len = strlen(pwent->pw_dir) \
            + strlen(SYSCONFDIR) + strlen("/.mpd.conf");

        mpdconf_path = (char*) malloc( sizeof(char) * mpdconf_path_len );
        if ( ! mpdconf_path ){
            fprintf( stderr, "%s:  Failed to allocate a buffer for path to mpd.conf\n", argv[0]);
            exit(1);
        }
        snprintf( mpdconf_path, mpdconf_path_len-1, "%s/.mpd.conf", pwent->pw_dir );
        if ( access( mpdconf_path, R_OK ) != 0 )
            snprintf( mpdconf_path, mpdconf_path_len-1, "%s/mpd.conf", SYSCONFDIR );
    }
    conf_file = fopen( mpdconf_path, "r");

    if (conf_file == NULL)
    {
        printf("%s: open failed for root's mpd conf file\n",argv[0]);
        exit(1);
    }
    secretword[0] = '\0';
    while (fgets(input_line,NAME_LEN,conf_file) != NULL)
    {
        input_line[strlen(input_line)-1] = '\0';  /* eliminate \n */
        if (input_line[0] == '#'  ||  input_line[0] == '\0')
            continue;
	if (strncmp(input_line,"secretword",10) == 0  &&  input_line[10] == '=')
	{
	    strncpy(secretword,&input_line[11],NAME_LEN);
	    secretword[NAME_LEN] = '\0';  /* just being cautious */
	}
	else if (strncmp(input_line,"MPD_SECRETWORD",14) == 0  &&  input_line[14] == '=')
	{
	    strncpy(secretword,&input_line[15],NAME_LEN);
	    secretword[NAME_LEN] = '\0';  /* just being cautious */
	}
    }
    if (secretword[0] == '\0')
    {
        printf("%s: did not find secretword in mpd conf file",argv[0]);
        exit(1);
    }

    /* setup default console */
    strncpy(console_name,argv[1],NAME_LEN);
    bzero( (void *)&sa, sizeof( sa ) );
    sa.sun_family = AF_UNIX;
    strncpy(sa.sun_path,console_name, sizeof(sa.sun_path)-1 );
    sock = atoi(argv[2]);
    rc = connect(sock,(struct sockaddr *)&sa,sizeof(sa));
    if (rc < 0)
    {
	perror("mpdroot: perror msg");
	printf("mpdroot: cannot connect to local mpd at: %s\n", console_name);
	printf("    probable cause:  no mpd daemon on this machine\n");
	printf("    possible cause:  unix socket %s has been removed\n", console_name);
	exit(1);
    }

    snprintf(cmd,NAME_LEN,"realusername=%s secretword=%s\n",pwent->pw_name,secretword);
    write(sock,cmd,strlen(cmd));

    if (mpdconf_path)
        free(mpdconf_path);

    return(0);
}

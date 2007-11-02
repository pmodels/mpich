/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int n, pmi_rank, pmi_port, pmi_sock;
    char *p, msg_to_send[1024], msg_recvd[1024], pmi_host_port[1024], kvsname[80];

    setbuf(stdout,NULL);
    pmi_rank = atoi(getenv("PMI_RANK"));
    strcpy(pmi_host_port,getenv("PMI_PORT"));
    printf("%d: pmi_rank=%d pmi_host_port=%s\n",getpid(),pmi_rank,pmi_host_port);
    p = strchr(pmi_host_port,':');
    pmi_port = atoi(p+1);
    *p = '\0';
    pmi_sock = connect_to_pm(pmi_host_port,pmi_port);

    if (argc > 1)
    {
	printf("%d: I must have been spawned with arg1=:%s:\n",getpid(),argv[1]);
	exit(0);
    }

    strcpy(msg_to_send,"cmd=init pmi_version=1 pmi_subversion=1\n");  /* PMI_VERSION here */
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent init\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);


#define TEST_THESE
#ifdef TEST_THESE
    strcpy(msg_to_send,"cmd=get_maxes\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent get_maxes\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);

    strcpy(msg_to_send,"cmd=get_my_kvsname\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent get_my_kvsname\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
    strcpy(kvsname, &msg_recvd[23]); 
    kvsname[strlen(kvsname)-1] = '\0';
    printf("kvsname = :%s:\n", kvsname );

    /* do puts before barrier*/
    sprintf(msg_to_send,"cmd=put kvsname=%s key=ralph value=butler\n", kvsname);
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent key=ralph\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);

    /* must recv barrier_out before doing gets from the kvs */
    strcpy(msg_to_send,"cmd=barrier_in\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent barrier_in\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);

    sprintf(msg_to_send,"cmd=get_universe_size\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent get for universe size\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
	
    sprintf(msg_to_send,"cmd=get kvsname=%s key=ralph\n", kvsname);
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent get for key ralph\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
	
    /* interface may have changed
    sprintf(msg_to_send,"cmd=spawn nprocs=1 totspawns=1 spawnssofar=1 preput_num=0 info_num=0 execname=./pmitest argcnt=1 arg1=spawned\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent spawn\n");
    ***/
#endif

    sprintf(msg_to_send,"cmd=publish_name service=rmb port=9999\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent publish_name\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
	
    sprintf(msg_to_send,"cmd=lookup_name service=rmb\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent lookup_name\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
	
    sprintf(msg_to_send,"cmd=unpublish_name service=rmb\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
    printf("sent unpublish_name\n");
    n = read(pmi_sock,msg_recvd,1024);
    if (n >= 0)
        msg_recvd[n] = '\0';
    printf("%d: recvd msg=:%s:\n",pmi_rank,msg_recvd);
	
    strcpy(msg_to_send,"cmd=finalize\n");
    write(pmi_sock,msg_to_send,strlen(msg_to_send));
}

int connect_to_pm( char *hostname, int portnum )
{
    struct hostent     *hp;
    struct sockaddr_in sin;
    int sfd, optval = 1;
    
    hp = gethostbyname( hostname );
    if (!hp)
    {
        printf("gethostbyname failed\n");
	exit(-1);
    }
    
    bzero( (void *)&sin, sizeof(sin) );
    bcopy( (void *)hp->h_addr, (void *)&sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port   = htons( (unsigned short) portnum );
    
    sfd = socket( AF_INET, SOCK_STREAM, 0 );
    if (sfd < 0)
    {
        printf("socket failed\n");
	exit(-1);
    }
    
    if (setsockopt( sfd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval) ))
    {
	perror( "Error calling setsockopt:" );
    }

    if (connect( sfd, (struct sockaddr *)&sin, sizeof(sin) ) < 0)
    {
        printf("connect failed\n");
	exit(-1);
    }

    return sfd;
}


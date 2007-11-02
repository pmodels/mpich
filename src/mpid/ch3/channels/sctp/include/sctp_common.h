
#if !defined(SCTP_COMMON_H_INCLUDED)
#define SCTP_COMMON_H_INCLUDED

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_ARPA_INET_H
/* Include this for inet_pton prototype */
#include <arpa/inet.h>
#endif

#define SOCK_ERROR -1

#define CHUNK 64*1024

/* open a scto many-to-one socket */
int sctp_open_dgm_socket2(int num_stream, int block_mode,
			 int listen_back_log, int port, int nagle,
			 int* buffer_size, struct sctp_event_subscribe* evnts,
			 int* fd, int* real_port);

int sctp_open_dgm_socket(void);

ssize_t sctp_writev(int s, struct iovec *data, int iovcnt,const
		    struct sockaddr *to,
		    socklen_t tolen __attribute__((unused)),
		    unsigned long ppid,
		    unsigned long flags,
		    unsigned short stream_no,
		    unsigned long timetolive,
		    unsigned long context,
		    int total );

int giveMeSockAddr(unsigned char ifaddr[], int port, struct sockaddr_in* addr);

int sctp_recv(int fd, char* buffer, int cnt, struct sctp_sndrcvinfo* sri, 
	      int* msg_flags, int* read);
int my_sctp_send(int fd, char* buffer, int cnt, struct sockaddr *to, uint16_t stream, int ppid);
int sctp_setblock(int fd, int mode);



#endif

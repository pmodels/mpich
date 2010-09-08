/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#ifdef HAVE_MPICHCONF_H
#include "mpichconf.h"
#else
#include "mpe_misc_conf.h"
#endif
#include "mpe_misc.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#if defined(HAVE_UNAME)
#include <sys/utsname.h>
#endif
#if defined(HAVE_GETHOSTBYNAME)
#if defined(HAVE_NETDB_H)
/* Some Solaris systems can't compile netdb.h */
#include <netdb.h>
#else
#undef HAVE_GETHOSTBYNAME
#endif
#endif /* HAVE_GETHOSTBYNAME */
#if defined(HAVE_SYSINFO)
#if defined(HAVE_SYS_SYSTEMINFO_H)
#include <sys/systeminfo.h>
#else
#ifdef HAVE_SYSINFO
#undef HAVE_SYSINFO
#endif
#endif
#endif

/*@
    MPE_GetHostName -  

    Parameters:
+ name : character pointer
- nlen : integer
@*/
void MPE_GetHostName( char *name, int nlen )
{
/* This is the prefered form, IF IT WORKS. */
#if defined(HAVE_UNAME) && defined(HAVE_GETHOSTBYNAME)
    struct utsname utname;
    struct hostent *he;
    uname( &utname );
    he = gethostbyname( utname.nodename );
    /* We must NOT use strncpy because it will null pad to the full length
       (nlen).  If the user has not allocated MPI_MAX_PROCESSOR_NAME chars,
       then this will unnecessarily overwrite storage.
     */
    /* strncpy(name,he->h_name,nlen); */
    {
	char *p_out = name;
	char *p_in  = he->h_name;
	int  i;
	for (i=0; i<nlen-1 && *p_in; i++) 
	    *p_out++ = *p_in++;
	*p_out = 0;
    }
#else
#if defined(HAVE_UNAME)
    struct utsname utname;
    uname(&utname);
    /* We must NOT use strncpy because it will null pad to the full length
       (nlen).  If the user has not allocated MPI_MAX_PROCESSOR_NAME chars,
       then this will unnecessarily overwrite storage.
     */
    /* strncpy(name,utname.nodename,nlen); */
    {
	char *p_out = name;
	char *p_in  = utname.nodename;
	int  i;
	for (i=0; i<nlen-1 && *p_in; i++) 
	    *p_out++ = *p_in++;
	*p_out = 0;
    }
#elif defined(HAVE_GETHOSTNAME)
    gethostname( name, nlen );
#elif defined(HAVE_SYSINFO)
    sysinfo(SI_HOSTNAME, name, nlen);
#else 
    strncpy( name, "Unknown!", nlen );
#endif
/* See if this name includes the domain */
    if (!strchr(name,'.')) {
    int  l;
    l = strlen(name);
    name[l++] = '.';
    name[l] = 0;  /* In case we have neither SYSINFO or GETDOMAINNAME */
#if defined(HAVE_SYSINFO) && defined(SI_SRPC_DOMAIN)
    sysinfo( SI_SRPC_DOMAIN,name+l,nlen-l);
#elif defined(HAVE_GETDOMAINNAME)
    getdomainname( name+l, nlen - l );
#endif
    }
#endif
}

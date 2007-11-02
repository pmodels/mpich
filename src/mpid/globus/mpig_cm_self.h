/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED)
#define MPICH2_MPIG_CM_SELF_H_INCLUDED

/*
 * expose the communication module's vtable so that it is accessible to other modules in the device
 */
extern struct mpig_cm mpig_cm_self;


/*
 * define the structure to be included in the communication method structure (CMS) of the VC object
 */
#define MPIG_VC_CMS_SELF_DECL						\
struct mpig_cm_self_vc_cms						\
{									\
    /* name of hostname running the process */				\
    char * hostname;							\
									\
    /* process id of the process */					\
    unsigned long pid;							\
}									\
self;


/*
 * global function prototypes
 */
#endif /* !defined(MPICH2_MPIG_CM_SELF_H_INCLUDED) */

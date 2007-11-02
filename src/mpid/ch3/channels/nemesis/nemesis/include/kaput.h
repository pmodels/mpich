/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * (C) 2001 Clemson University and The University of Chicago
 *
 * See COPYING in top-level directory.
 */

#ifndef __KAPUT_H
#define __KAPUT_H

#ifdef __KERNEL__
#include <linux/uio.h>
#else
#include <sys/uio.h>
#include <sys/ioctl.h>
#endif

typedef long kaput_token;

#define KAPUT_PIN 1
struct kaput_pin
{
    /* address and size that we wish to pin */
    void* in_addr;
    int in_size;
    
    /* token that can then be used to access or release region */
    kaput_token out_token;
};

#define KAPUT_RELEASE 2
struct kaput_release
{
    /* token associated with the region */
    kaput_token in_token;
};

#define KAPUT_READ 3
#define KAPUT_WRITE 4
struct kaput_io
{
    void* in_addr;
    int in_size;
    /* token associated with target buffer */
    kaput_token in_token;
};

#ifndef __KERNEL__
/* wrapper functions */

static inline int
kaput_open (int *kaput_fd)
{
    *kaput_fd = open("/dev/kaput", O_RDWR);

    return (kaput_fd < 0) ? -1 : 0;
}

static inline int
kaput_register (int kaput_fd, void *addr, int size, kaput_token *token)
{
    int ret;
    struct kaput_pin info;
    
    info.in_addr = addr;
    info.in_size = size;
    ret = ioctl (kaput_fd, KAPUT_PIN, &info);
    *token = info.out_token;
    return (ret == 0) ? 0 : -1;
}

static inline int
kaput_deregister (int kaput_fd, kaput_token token)
{
    int ret;
    struct kaput_release info;
    
    info.in_token = token;
    ret = ioctl (kaput_fd, KAPUT_RELEASE, &info);
    return (ret == 0) ? 0 : -1;
}

static inline int
kaput_put (int kaput_fd, void *addr, int size, kaput_token token)
{
    int ret;
    struct kaput_io info;
    
    info.in_addr = addr;
    info.in_size = size;
    info.in_token = token;
    ret = ioctl (kaput_fd, KAPUT_WRITE, &info);
    return (ret == 0) ? 0 : -1;
}

static inline int
kaput_get (int kaput_fd, void *addr, int size, kaput_token token)
{
    int ret;
    struct kaput_io info;
    
    info.in_addr = addr;
    info.in_size = size;
    info.in_token = token;
    ret = ioctl (kaput_fd, KAPUT_READ, &info);
    return (ret == 0) ? 0 : -1;
}
#endif
#endif /* __KAPUT_H */

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <scif.h>
#include <assert.h>
#include <sys/uio.h>
#include <string.h>
#include "scifrw.h"
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

extern int MPID_nem_scif_myrank;

int MPID_nem_scif_init_shmsend(shmchan_t * csend, int ep, int rank)
{
    int retval = 0;
/* FIXME: correctly detect getpagesize */
#if defined HAVE_GETPAGESIZE || 1
    int pagesize = getpagesize();
#else
#error need getpagesize
#endif
    off_t offset;
    void *p;

    assert(csend->buflen == 0);
    retval = posix_memalign(&p, pagesize, SHMBUFSIZE);
    if (retval != 0)
        goto fn_exit;
    offset = scif_register(ep, p, SHMBUFSIZE, 0, SCIF_PROT_READ | SCIF_PROT_WRITE, 0);
    if (offset == SCIF_REGISTER_FAILED) {
        retval = errno;
        free(p);
        goto fn_exit;
    }
    csend->bufp = p;
    csend->buflen = SHMBUFSIZE;
    csend->offset = offset;
    csend->seqno = 10 * MPID_nem_scif_myrank;
    csend->lseqno = (uint64_t *) csend->bufp;
    csend->rseqno = csend->lseqno + 1;
    *csend->lseqno = *csend->rseqno = csend->seqno;
    csend->curp = csend->bufp + 2 * sizeof(uint64_t);
    csend->pos = -1;
    csend->reg = 0;
    csend->rank = rank;
    csend->dma_count = 0;
    csend->dma_chdseqno = 0;

  fn_exit:
    return retval;
}

int MPID_nem_scif_init_shmrecv(shmchan_t * crecv, int ep, off_t offs, int rank)
{
    int retval = 0;

    crecv->bufp = scif_mmap(0, SHMBUFSIZE, SCIF_PROT_READ | SCIF_PROT_WRITE, 0, ep, offs);
    if (crecv->bufp == SCIF_MMAP_FAILED) {
        retval = errno;
        goto fn_exit;
    }
    crecv->buflen = SHMBUFSIZE;
    crecv->pos = -1;
    crecv->offset = offs;
    crecv->seqno = 10 * rank;
    crecv->lseqno = (uint64_t *) crecv->bufp;
    crecv->rseqno = crecv->lseqno + 1;
    crecv->curp = crecv->bufp + 2 * sizeof(uint64_t);
    crecv->reg = 0;
    crecv->dmalen = 0;
    crecv->dmaoffset = 0;
    crecv->dmastart = -1;
    crecv->dmaend = 0;
    crecv->rank = rank;
  fn_exit:
    return retval;
}

void MPID_nem_scif_unregmem(int ep, shmchan_t * c)
{
    regmem_t *rp = c->reg, *prev = c->reg;
    uint64_t lseqno = c->dma_chdseqno;

    while (rp && c->dma_count) {
        if (rp->seqno <= lseqno) {
            scif_unregister(ep, rp->offset, rp->size);
            prev->next = rp->next;
            if (c->reg == rp) {
                c->reg = rp->next;
            }
            free(rp);
            --c->dma_count;
            rp = prev->next;
        }
        else {
            prev = rp;
            rp = rp->next;
        }
    }
}

static regmem_t *regmem(int ep, shmchan_t * c, void *addr, size_t len)
{
    regmem_t *rp;
    uint64_t base, size;
/* FIXME: correctly detect getpagesize */
#if defined HAVE_GETPAGESIZE || 1
    int pagesize = getpagesize();
#else
#error need getpagesize
#endif

    /*
     * FIXME: if the user unmaps this address and then it gets
     * remapped to a different physical address, then this address
     * won't be pinned anymore. Hopefully we'll just get a dma
     * failure.
     */

    base = (uint64_t) addr & ~(pagesize - 1);
    size = ((uint64_t) addr + len + pagesize - 1) & ~(pagesize - 1);
    size -= base;

    rp = malloc(sizeof(regmem_t));
    rp->base = (char *) base;
    rp->size = size;
    rp->offset = scif_register(ep, (void *) base, size, 0, SCIF_PROT_READ | SCIF_PROT_WRITE, 0);
    if (rp->offset == SCIF_REGISTER_FAILED) {
        fprintf(stderr, "regmem failed: errno %d base 0x%lx size %ld\n", errno, base, size);
        free(rp);
        return 0;
    }
    rp->next = c->reg;
    c->reg = rp;
    return rp;
}

static int dma_read(int ep, shmchan_t * c, void *recv_buf, off_t raddr, size_t msglen,
                    size_t buflen, int *did_dma)
{
    regmem_t *rp;
    int retval = 0;
    off_t locoffs;
    size_t newbufsiz;
    size_t firstcacheline;
    size_t rem;
    int mark;
/* FIXME: correctly detect getpagesize */
#if defined HAVE_GETPAGESIZE || 1
    int pagesize = getpagesize();
#else
#error need getpagesize
#endif

    /* if we have any left over from the last DMA */
    if (c->dmastart >= 0) {
        rem = c->dmaend - c->dmastart - c->pos;
        if (rem > buflen)
            rem = buflen;
        memcpy(recv_buf, c->dmabuf + c->dmastart + c->pos, rem);
        if (c->dmastart + c->pos + rem >= c->dmaend)
            c->dmastart = -1;
        goto fn_exit;
    }

    /* see if we can DMA into the destination */
    if (buflen >= msglen &&
        ((off_t) recv_buf & (CACHE_LINESIZE - 1)) == ((raddr + c->pos) & (CACHE_LINESIZE - 1))) {

        retval = scif_vreadfrom(ep, recv_buf, buflen, raddr + c->pos, 0);
        if (retval < 0) {
            fprintf(stderr, "scif_vreadfrom #1 returns %d, errno %d\n", retval, errno);
            fprintf(stderr, "recv_buf: %p raddr: 0x%lx buflen: %ld\n",
                    recv_buf, raddr + c->pos, buflen);
        }
        scif_fence_mark(ep, SCIF_FENCE_INIT_SELF, &mark);
        scif_fence_wait(ep, mark);
        *did_dma = 1;
        goto fn_exit;
    }

    /* DMA into a local buffer and copy */
    newbufsiz = (msglen + pagesize + pagesize - 1) & ~(pagesize - 1);
    if (newbufsiz < DMABUFSIZE)
        newbufsiz = DMABUFSIZE;
    if (newbufsiz > c->dmalen) {
        void *p;
        off_t offset;
        if (c->dmalen) {
            free(c->dmabuf);
        }
        retval = posix_memalign(&p, pagesize, newbufsiz);
        if (retval != 0)
            goto fn_exit;

        c->dmabuf = p;
        c->dmalen = newbufsiz;
        c->dmaoffset = 0;
        c->dmastart = -1;
        c->dmaend = 0;
    }

    /* get the alignment right */
    firstcacheline = (raddr + (CACHE_LINESIZE - 1)) & ~(CACHE_LINESIZE - 1);
    c->dmastart = CACHE_LINESIZE - (firstcacheline - raddr);
    c->dmaend = c->dmastart + msglen;
    if (c->dmaend > c->dmalen) {
        fprintf(stderr, "%d: botch len c->dmastart %ld c->dmaend %ld c->dmalen %ld\n",
                MPID_nem_scif_myrank, c->dmastart, c->dmaend, c->dmalen);
        assert(0);
    }
    locoffs = c->dmaoffset + c->dmastart;
    assert(c->pos == 0);
    retval = scif_vreadfrom(ep, c->dmabuf + c->dmastart, msglen, raddr, 0);
    *did_dma = 1;
    scif_fence_mark(ep, SCIF_FENCE_INIT_SELF, &mark);
    scif_fence_wait(ep, mark);
    if (retval < 0)
        fprintf(stderr, "scif_readfrom #2 returns %d, errno %d\n", retval, errno);
    rem = c->dmaend - c->dmastart;
    if (rem > buflen)
        rem = buflen;
    memcpy(recv_buf, c->dmabuf + c->dmastart, rem);
    if (c->dmastart + rem >= c->dmaend)
        c->dmastart = -1;

  fn_exit:
    return retval;
}

static ssize_t getmsg(int ep, shmchan_t * c, void *recv_buf, size_t len, int *did_dma)
{
    int64_t msglen;
    int largemsg = 0;
    size_t tlen;
    int err;
    ssize_t retval;

    msglen = *(int64_t *) c->curp;

    if (msglen == -1) {
        /* go to beginning of buffer */
        c->curp = c->bufp + 2 * sizeof(uint64_t);
        msglen = *(int64_t *) c->curp;
    }
    if (msglen < 0) {
        largemsg = 1;
        msglen = -msglen;
    }
    tlen = msglen - c->pos;
    if (tlen > len)
        tlen = len;
    if (!largemsg)
        memcpy(recv_buf, c->curp + sizeof(int64_t) + c->pos, tlen);
    else {
        err = dma_read(ep, c, recv_buf, ((off_t *) c->curp)[1], msglen, tlen, did_dma);
        if (err) {
            retval = -1;
            goto fn_exit;
        }
    }
    c->pos += tlen;
    assert(c->pos <= msglen);
    retval = tlen;
    if (c->pos == msglen) {
        /* Done with this message */
        c->pos = -1;
        if (!largemsg)
            c->curp += (sizeof(int64_t) + ((msglen + 7) & ~7));
        else
            c->curp += (2 * sizeof(int64_t));
    }
  fn_exit:
    return retval;
}

/* Read at most one message */

static ssize_t do_scif_read(int ep, shmchan_t * c, void *recv_buf, size_t len, int *did_dma)
{
    ssize_t retval = 0;
    size_t nread = 0;
    uint64_t rseqno;

    while (nread < len) {
        if (c->pos >= 0) {
            /* partial message chunk left */
            retval = getmsg(ep, c, (char *) recv_buf + nread, len - nread, did_dma);
            if (retval < 0) {
                nread = -1;
                goto fn_exit;
            }
            nread += retval;
            continue;
        }
        /* Check if we have a message */
        rseqno = *c->rseqno;
        if (rseqno <= c->seqno) {
            goto fn_exit;
        }
        /* Message is available */
        ++c->seqno;
        c->pos = 0;
        retval = getmsg(ep, c, (char *) recv_buf + nread, len - nread, did_dma);
        if (retval < 0) {
            nread = -1;
            goto fn_exit;
        }
        nread += retval;
    }

  fn_exit:
    return nread;
}

ssize_t MPID_nem_scif_read(int ep, shmchan_t * c, void *recv_buf, size_t len)
{
    int did_dma = 0;
    ssize_t retval;
    int mark;

    retval = do_scif_read(ep, c, recv_buf, len, &did_dma);
    if (retval > 0 && did_dma) {
        scif_fence_mark(ep, SCIF_FENCE_INIT_SELF, &mark);
        scif_fence_wait(ep, mark);
    }
    /* Tell the other side we're done */
    *c->lseqno = c->seqno;
    if (retval <= 0)
        fprintf(stderr, "%2d: %2d: %08ld: %08ld: readv err %ld\n",
                MPID_nem_scif_myrank, c->rank, c->seqno, *c->rseqno, retval);
    return retval;
}

ssize_t MPID_nem_scif_readv(int ep, shmchan_t * c, const struct iovec * iov, int iov_cnt)
{
    ssize_t retval = 0;
    size_t nread = 0;
    int did_dma = 0;
    int mark;
    int i;

    for (i = 0; i < iov_cnt; ++i) {
        retval = do_scif_read(ep, c, iov[i].iov_base, iov[i].iov_len, &did_dma);
        if (retval < 0) {
            nread = retval;
            goto fn_exit;
        }
        if (retval == 0)
            break;
        nread += retval;
        if (retval < iov[i].iov_len)
            break;
    }
    if (retval > 0 && did_dma) {
        scif_fence_mark(ep, SCIF_FENCE_INIT_SELF, &mark);
        scif_fence_wait(ep, mark);
    }
  fn_exit:
    /* Tell the other side we're done */
    *c->lseqno = c->seqno;
    if (nread <= 0)
        fprintf(stderr, "%5d: %08ld: readv err %ld\n", getpid(), c->seqno, nread);
    return nread;
}

int MPID_nem_scif_poll_send(int ep, shmchan_t * csend)
{
    size_t avail;
    int retval = 1;
    uint64_t lseqno;

    avail = csend->buflen - (csend->curp - csend->bufp) - sizeof(uint64_t);
    if (avail >= SMALLMSG)
        goto fn_exit;   /* room for a message */
    lseqno = *csend->lseqno;
    if (lseqno == csend->seqno)
        goto fn_exit;   /* remote side has consumed everything */
    assert(lseqno < csend->seqno);
    retval = 0;
  fn_exit:
    return retval;
}

ssize_t MPID_nem_scif_writev(int ep, shmchan_t * c, const struct iovec * iov, int iov_cnt,
                             uint64_t * seqno)
{
    size_t nwritten = 0;
    int did_dma = 0;
    int i;
    int mark;
    regmem_t *rp;

    for (i = 0; i < iov_cnt; ++i) {
        size_t len;
        int smallmsg = 0;
        size_t iovlen;
        size_t avail;

        iovlen = iov[i].iov_len;
        if (iovlen == 0)
            continue;
        avail = c->buflen - (c->curp - c->bufp) - sizeof(uint64_t);
        if (iovlen >= SMALLMSG)
            len = 2 * sizeof(int64_t);
        else {
            smallmsg = 1;
            len = sizeof(int64_t) + ((iovlen + 7) & ~7);
        }
        if (len > avail) {
            uint64_t lseqno = *c->lseqno;
            *(int64_t *) c->curp = ~0l;
            if (lseqno != c->seqno) {
                if (nwritten == 0)
                    fprintf(stderr, "%5d: %08ld: lseqno %08ld len %ld writev reset buffer\n",
                            getpid(), c->seqno, lseqno, len);
                assert(lseqno < c->seqno);
                goto fn_exit;
            }
            /* Remote side has consumed everything, we can reset the buffer */
            c->curp = c->bufp + 2 * sizeof(uint64_t);
        }
        if (smallmsg) {
            *(int64_t *) c->curp = iovlen;
            memcpy(c->curp + sizeof(int64_t), iov[i].iov_base, iovlen);
        }
        else {
            did_dma = 1;
            rp = regmem(ep, c, iov[i].iov_base, iovlen);
            if (rp == 0) {
                nwritten = -1;
                goto fn_exit;
            }
            *(int64_t *) c->curp = -iovlen;
            ((uint64_t *) c->curp)[1] = rp->offset + ((char *) iov[i].iov_base - rp->base);
        }
        c->curp += len;
        nwritten += iovlen;
        ++c->seqno;

        if (did_dma) {
            rp->seqno = c->seqno;
            ++c->dma_count;
        }
    }
  fn_exit:
#if !defined(__MIC__)
    _mm_mfence();
#endif
    /* Tell the other side it's available */
    *c->rseqno = c->seqno;
    if (did_dma)
        *seqno = c->seqno;
    else
        *seqno = 0;
    if (nwritten <= 0)
        fprintf(stderr, "%5d: %08ld: writev err %ld\n", getpid(), c->seqno, nwritten);
    return nwritten;
}

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

#ifndef SCIFRW_H
#define SCIFRW_H

#define SHMBUFSIZE (64*1024)
#define DMABUFSIZE (1024*1024)
#define SMALLMSG   (4096)

#define NPOLLS     (100)
#define CACHE_LINESIZE (64)

/* registered memory table entry */
typedef struct regmem {
    char *base;
    size_t size;
    off_t offset;               /* for DMA */
    uint64_t seqno;             /* sender sequence number, */
    /* used for unreg */
    struct regmem *next;
} regmem_t;

/* Shared memory channel */
typedef struct {
    uint64_t seqno;             /* my current sequence number */
    off_t offset;               /* for DMA */
    /* Sequence numbers. rseqno is remote for receiver, lseqno is
     * remote for sender.
     */
    volatile uint64_t *rseqno;
    volatile uint64_t *lseqno;
    char *bufp;                 /* the mmap'd buffer */
    size_t buflen;              /* and its size */
    char *curp;                 /* where I am currently */
    ssize_t pos;                /* position in current packet (receive only) */
    char *dmabuf;               /* buffer for large DMA */
    size_t dmalen;              /* and its size */
    off_t dmaoffset;            /* offset for DMA buffer */
    ssize_t dmastart;           /* start of the read in the DMA buffer */
    ssize_t dmaend;             /* end of the read in the DMA buffer */
    regmem_t *reg;              /* registration list */
    int rank;                   /* mostly for debugging */
    int dma_count;              /* count of DMA ops (send only) */
    int dma_chdseqno;           /* last checked segno           */
} shmchan_t;

int MPID_nem_scif_init_shmsend(shmchan_t * csend, int ep, int rank);
int MPID_nem_scif_init_shmrecv(shmchan_t * crecv, int ep, off_t offs, int rank);

ssize_t MPID_nem_scif_read(int ep, shmchan_t * c, void *recv_buf, size_t len);
ssize_t MPID_nem_scif_readv(int ep, shmchan_t * c, const struct iovec *iov, int iov_cnt);
ssize_t MPID_nem_scif_writev(int ep, shmchan_t * c, const struct iovec *iov, int iov_cnt,
                             uint64_t * seqno);

int MPID_nem_scif_poll_send(int ep, shmchan_t * csend);

static inline int MPID_nem_scif_poll_recv(shmchan_t * crecv)
{
    uint64_t rseqno = *crecv->rseqno;
    return crecv->pos != -1 || crecv->seqno < rseqno;
}


static inline int MPID_nem_scif_chk_seqno(shmchan_t * csend, int seqno)
{
    return *csend->lseqno >= seqno;
}

static inline int MPID_nem_scif_chk_dma_unreg(shmchan_t * csend)
{
    uint64_t lseqno = *csend->lseqno;

    if (lseqno != csend->dma_chdseqno) {
        csend->dma_chdseqno = lseqno;
        return csend->dma_count;
    }
    return 0;
}
#endif

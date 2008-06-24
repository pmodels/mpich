/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/pt2pt/mpidi_callback_ssm.c
 * \brief The standard callback for a new short message
 */
#include "mpidimpl.h"

/**
 * \brief Callback for incoming SSM CTS message.
 * \note  Because this is a short message, the data is already received
 * \param[in]  clientdata Unused
 * \param[in]  msginfo    The 16-byte msginfo struct
 * \param[in]  count      The number of msginfo quads (1)
 * \param[in]  senderrank The sender's rank
 * \param[in]  sndlen     The length of the incoming data
 * \param[in]  sndbuf     Where the data is stored
 */
void MPIDI_BG2S_SsmCtsCB(void                     * clientdata,
                         const MPIDI_DCMF_MsgInfo * msginfo,
                         unsigned                   count,
                         unsigned                   senderrank,
                         const char               * sndbuf,
                         unsigned                   sndlen)
{
  SSM_ABORT();
}


/**
 * \brief Callback for SSM Ack to indicate that the put is done.
 * \note  Because this is a short message, the data is already received
 * \param[in]  clientdata Unused
 * \param[in]  msginfo    The 16-byte msginfo struct
 * \param[in]  count      The number of msginfo quads (1)
 * \param[in]  senderrank The sender's rank
 * \param[in]  sndlen     The length of the incoming data
 * \param[in]  sndbuf     Where the data is stored
 */
void MPIDI_BG2S_SsmAckCB(void                     * clientdata,
                         const MPIDI_DCMF_MsgInfo * msginfo,
                         unsigned                   count,
                         unsigned                   senderrank,
                         const char               * sndbuf,
                         unsigned                   sndlen)
{
  SSM_ABORT();
}


/**
 * \brief Message layer callback which is invoked on the origin node
 * when the message put is done
 *
 * \param[in,out] sreq MPI receive request object
 */
void MPIDI_DCMF_SsmPutDoneCB (MPID_Request * sreq)
{
  SSM_ABORT();
}

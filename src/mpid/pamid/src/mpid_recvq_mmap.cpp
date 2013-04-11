#include <map>
#include <stdio.h>
#include <cstdlib>
#include "mpidi_platform.h"


extern "C" { 

struct MPID_Queue_map_key_t
{
  int context_id;
  int source;
  int tag;
  inline bool operator< (const MPID_Queue_map_key_t& qmk) const
  {
    if(context_id < qmk.context_id)
      return true;
    else if(context_id == qmk.context_id)
    {
      if(source < qmk.source)
        return true;
      else if(source == qmk.source)
      {
        if(tag < qmk.tag)
          return true;
        else
          return false;
      }
      else
        return false;
    }
    else
      return false;
  } 
};

struct MPID_Queue_map_value_t
{
  void* rreq;
#ifdef OUT_OF_ORDER_HANDLING
  int seqno;
#endif
};

typedef std::multimap<MPID_Queue_map_key_t,MPID_Queue_map_value_t> MPID_Req_queue_map_t;
typedef std::multimap<MPID_Queue_map_key_t,MPID_Queue_map_value_t>::iterator MPID_Req_queue_map_iterator_t;

MPID_Req_queue_map_t MPID_Unexp_queue;
MPID_Req_queue_map_t MPID_Posted_queue;

MPID_Req_queue_map_iterator_t itp;
MPID_Req_queue_map_iterator_t itu;

void MPIDI_Recvq_init_queues();
int MPIDI_Recvq_empty_uexp();
int MPIDI_Recvq_empty_post();

#ifndef OUT_OF_ORDER_HANDLING
void MPIDI_Recvq_insert_uexp(void * rreq, int source, int tag, int context_id);
void MPIDI_Recvq_insert_post(void * rreq, int source, int tag, int context_id);
void MPIDI_Recvq_insrt(MPID_Req_queue_map_t* queue, void * rreq, int source, int tag, int context_id);

void MPIDI_Recvq_remove_uexp(int source, int tag, int context_id, void* it_req);
void MPIDI_Recvq_remove_post(int source, int tag, int context_id, void* it_req);
void MPIDI_Recvq_rm(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, void* it_req);

void MPIDI_Recvq_find_in_uexp(int source, int tag, int context_id, void** req, void** it_req);
void MPIDI_Recvq_find_in_post(int source, int tag, int context_id, void** req, void** it_req);
void MPIDI_Recvq_find(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, void** req, void** it_req);

#else
void MPIDI_Recvq_insert_uexp(void * rreq, int source, int tag, int context_id, int seqno);
void MPIDI_Recvq_insert_post(void * rreq, int source, int tag, int context_id);
void MPIDI_Recvq_insrt(MPID_Req_queue_map_t* queue, void * rreq, int source, int tag, int context_id, int seqno);

void MPIDI_Recvq_remove_uexp(int source, int tag, int context_id, int seqno, void* it_req);
void MPIDI_Recvq_remove_uexp_noit(int source, int tag, int context_id, int seqno);
void MPIDI_Recvq_remove_post(int source, int tag, int context_id, void* it_req);
void MPIDI_Recvq_rm(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, int seqno, void* it_req);

void MPIDI_Recvq_find_in_uexp(int source, int tag, int context_id, int seqno, void** req, void** it_req);
void MPIDI_Recvq_find_in_post(int source, int tag, int context_id, void** req, void** it_req);
void MPIDI_Recvq_find(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, int seqno, void** req, void** it_req);

#endif




void MPIDI_Recvq_init_queues()
{
  MPID_Unexp_queue.clear();
  MPID_Posted_queue.clear();
}

int MPIDI_Recvq_empty_uexp()
{
  return MPID_Unexp_queue.empty();
}

int MPIDI_Recvq_empty_post()
{
  return MPID_Posted_queue.empty();
}

#ifndef OUT_OF_ORDER_HANDLING
void MPIDI_Recvq_insert_uexp(void * rreq, int source, int tag, int context_id)
{
  MPIDI_Recvq_insrt(&MPID_Unexp_queue, rreq, source, tag, context_id);
}


void MPIDI_Recvq_insert_post(void * rreq, int source, int tag, int context_id)
{
  MPIDI_Recvq_insrt(&MPID_Posted_queue, rreq, source, tag, context_id);
}


void MPIDI_Recvq_insrt(MPID_Req_queue_map_t* queue, void * rreq, int source, int tag, int context_id)
{
  MPID_Queue_map_key_t key;
  MPID_Queue_map_value_t value;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  value.rreq     = rreq;
  queue->insert(std::make_pair(key,value));  
}


void MPIDI_Recvq_remove_uexp(int source, int tag, int context_id, void* it_req)
{
  MPIDI_Recvq_rm(&MPID_Unexp_queue, source, tag, context_id, it_req);
}


void MPIDI_Recvq_remove_post(int source, int tag, int context_id, void* it_req)
{
  MPIDI_Recvq_rm(&MPID_Posted_queue, source, tag, context_id, it_req);
}


void MPIDI_Recvq_rm(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, void* it_req)
{
  MPID_Req_queue_map_iterator_t it = *((MPID_Req_queue_map_iterator_t*)it_req);
  queue->erase(it);
}


void MPIDI_Recvq_find_in_uexp(int source, int tag, int context_id, void** req, void** it_req)
{
  return MPIDI_Recvq_find(&MPID_Unexp_queue, source, tag, context_id, req, it_req);
}

void MPIDI_Recvq_find_in_post(int source, int tag, int context_id, void** req, void** it_req)
{
  MPID_Queue_map_key_t key;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  *it_req = NULL;
  *req    = NULL;
  itp = MPID_Posted_queue.find(key);
  if(itp != MPID_Posted_queue.end())
  {
    *it_req = &itp;
    *req = ((MPID_Queue_map_value_t)(itp->second)).rreq;
  }
}

void MPIDI_Recvq_find(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, void** req, void** it_req)
{
  MPID_Queue_map_key_t key;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  *req = NULL;
  *it_req = NULL;
  itu = queue->find(key);
  if(itu != queue->end())
  {
    *req = ((MPID_Queue_map_value_t)(itu->second)).rreq;
    *it_req = &itu;
    return;
  }

  if(source < 0 && tag >= 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).tag == tag && ((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        *it_req = &itu;
        *req = ((MPID_Queue_map_value_t)itu->second).rreq;
        return;
      }
    }
  }
  else if(source >= 0 && tag < 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).source == source && ((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        *it_req = &itu;
        *req = ((MPID_Queue_map_value_t)itu->second).rreq;
        return;
      }
    }
  }
  else if(source < 0 && tag < 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        *it_req = &itu;
        *req = ((MPID_Queue_map_value_t)itu->second).rreq;
        return;
      }
    }
  }

}

#else
void MPIDI_Recvq_insert_uexp(void * rreq, int source, int tag, int context_id, int seqno)
{
  MPIDI_Recvq_insrt(&MPID_Unexp_queue, rreq, source, tag, context_id, seqno);

}


void MPIDI_Recvq_insert_post(void * rreq, int source, int tag, int context_id)
{
  MPIDI_Recvq_insrt(&MPID_Posted_queue, rreq, source, tag, context_id, -1);
}


void MPIDI_Recvq_insrt(MPID_Req_queue_map_t* queue, void * rreq, int source, int tag, int context_id, int seqno)
{
  MPID_Queue_map_key_t key;
  MPID_Queue_map_value_t value;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  value.seqno    = seqno;
  value.rreq     = rreq;
  queue->insert(std::make_pair(key,value));
}


void MPIDI_Recvq_remove_uexp(int source, int tag, int context_id, int seqno, void* it_req)
{
  MPIDI_Recvq_rm(&MPID_Unexp_queue, source, tag, context_id, seqno, it_req);
}

void MPIDI_Recvq_remove_uexp_noit(int source, int tag, int context_id, int seqno)
{
  MPID_Queue_map_key_t key;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  MPID_Req_queue_map_iterator_t it;
  std::pair <MPID_Req_queue_map_iterator_t, MPID_Req_queue_map_iterator_t > itpair;
  itpair = MPID_Unexp_queue.equal_range(key);
  for(it = itpair.first; itu != itpair.second; ++it)
    if(((MPID_Queue_map_value_t)it->second).seqno == seqno)
    {
      MPID_Unexp_queue.erase(it);
      break;
    }
}

void MPIDI_Recvq_remove_post(int source, int tag, int context_id, void* it_req)
{
  MPIDI_Recvq_rm(&MPID_Posted_queue, source, tag, context_id, -1, it_req);
}


void MPIDI_Recvq_rm(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, int seqno, void* it_req)
{
  MPID_Req_queue_map_iterator_t it = *((MPID_Req_queue_map_iterator_t*)it_req);
  queue->erase(it);
}


void MPIDI_Recvq_find_in_uexp(int source, int tag, int context_id, int seqno, void** req, void** it_req)
{
  return MPIDI_Recvq_find(&MPID_Unexp_queue, source, tag, context_id, seqno, req, it_req);
}

void MPIDI_Recvq_find_in_post(int source, int tag, int context_id, void** req, void** it_req)
{
  MPID_Queue_map_key_t key;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;
  *req = NULL;
  *it_req = NULL;
  itp = MPID_Posted_queue.find(key);
  if(itp!=MPID_Posted_queue.end())
  {
    *it_req = (void*)&itp;
    *req  = ((MPID_Queue_map_value_t)(itp->second)).rreq;
  }
}


void MPIDI_Recvq_find(MPID_Req_queue_map_t* queue, int source, int tag, int context_id, int seqno, void** req, void** it_req)
{
  MPID_Queue_map_key_t key;
  key.context_id = context_id;
  key.source     = source;
  key.tag        = tag;

  *req = NULL;
  *it_req = NULL;
  if(seqno == -1)
  {
    itu = queue->find(key);
    if(itu != queue->end())
    {
      *it_req = (void*)&itu;
      *req    = ((MPID_Queue_map_value_t)itu->second).rreq;
    }
  }
  else
  {
    std::pair <MPID_Req_queue_map_iterator_t, MPID_Req_queue_map_iterator_t > itpair;
    itpair = queue->equal_range(key);
    for(itu = itpair.first; itu != itpair.second; ++itu)
      if(((MPID_Queue_map_value_t)itu->second).seqno <= seqno)
      {
        *it_req = (void*)&itu;
        *req    = ((MPID_Queue_map_value_t)itu->second).rreq;
        break;
      }
  }
  if(*req != NULL)
    return;

  if(source < 0 && tag >= 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).tag == tag && ((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        if(seqno == -1)
        {
         *it_req = (void*)&itu;
         *req = ((MPID_Queue_map_value_t)itu->second).rreq;
          return;
        }
        else
        {
          if(((MPID_Queue_map_value_t)itu->second).seqno <= seqno)
          {
            *it_req = (void*)&itu;
            *req = ((MPID_Queue_map_value_t)itu->second).rreq;
            return;
          }
        }
      }
    }
  }
  else if(source >= 0 && tag < 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).source == source && ((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        if(seqno == -1)
        {
          *it_req = (void*)&itu;
          *req = ((MPID_Queue_map_value_t)itu->second).rreq;
          return;
        }
        else
        {
          if(((MPID_Queue_map_value_t)itu->second).seqno <= seqno)
          {
            *it_req = (void*)&itu;
            *req = ((MPID_Queue_map_value_t)itu->second).rreq;
            return;
          }
        }
      }
    }
  }
  else if(source < 0 && tag < 0)
  {
    for(itu = queue->begin(); itu != queue->end(); itu++)
    {
      if(((MPID_Queue_map_key_t)itu->first).context_id == context_id)
      {
        if(seqno == -1)
        {
          *it_req = (void*)&itu;
          *req = ((MPID_Queue_map_value_t)itu->second).rreq;
          return;
        }
        else
        {
          if(((MPID_Queue_map_value_t)itu->second).seqno <= seqno)
          {
            *it_req = (void*)&itu;
            *req = ((MPID_Queue_map_value_t)itu->second).rreq;
            return;
          }
        }
      }
    }
  }
}

#endif

}

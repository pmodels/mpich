/* MPICH2 notes: 
 * - The file name has been changed to avoid conflicts with any system-installed
 *   "utlist.h" header files.
 * - some configure-time checking for __typeof() support was added
 * - intentionally omitted from "mpl.h" in order to require using code to opt-in
 * [goodell@ 2010-12-20] */
/*
Copyright (c) 2007-2010, Troy D. Hanson   http://uthash.sourceforge.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MPL_UTLIST_H
#define MPL_UTLIST_H

#define MPL_UTLIST_VERSION 1.9.1

/* 
 * This file contains macros to manipulate singly and doubly-linked lists.
 *
 * 1. LL_ macros:  singly-linked lists.
 * 2. DL_ macros:  doubly-linked lists.
 * 3. CDL_ macros: circular doubly-linked lists.
 *
 * To use singly-linked lists, your structure must have a "next" pointer.
 * To use doubly-linked lists, your structure must "prev" and "next" pointers.
 * Either way, the pointer to the head of the list must be initialized to NULL.
 * 
 * ----------------.EXAMPLE -------------------------
 * struct item {
 *      int id;
 *      struct item *prev, *next;
 * }
 *
 * struct item *list = NULL:
 *
 * int main() {
 *      struct item *item;
 *      ... allocate and populate item ...
 *      DL_APPEND(list, item);
 * }
 * --------------------------------------------------
 *
 * For doubly-linked lists, the append and delete macros are O(1)
 * For singly-linked lists, append and delete are O(n) but prepend is O(1)
 * The sort macro is O(n log(n)) for all types of single/double/circular lists.
 */

#ifdef MPL_HAVE___TYPEOF /* MPICH2 modification */
/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ code), this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#ifdef _MSC_VER            /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define MPL_LDECLTYPE(x) decltype(x)
#else                     /* VS2008 or older (or VS2010 in C mode) */
#define MPL_NO_DECLTYPE
#define MPL_LDECLTYPE(x) char*
#endif
#else                      /* GNU, Sun and other compilers */
#define MPL_LDECLTYPE(x) __typeof(x)
#endif
#else /* !MPL_HAVE___TYPEOF */
#define MPL_NO_DECLTYPE
#define MPL_LDECLTYPE(x) char*
#endif /* !MPL_HAVE___TYPEOF */

/* for VS2008 we use some workarounds to get around the lack of decltype,
 * namely, we always reassign our tmp variable to the list head if we need
 * to dereference its prev/next pointers, and save/restore the real head.*/
#ifdef MPL_NO_DECLTYPE
#define MPL__SV(elt,list) _tmp = (char*)(list); {char **_alias = (char**)&(list); *_alias = (elt); }
#define MPL__NEXT(elt,list) ((char*)((list)->next))
#define MPL__NEXTASGN(elt,list,to) { char **_alias = (char**)&((list)->next); *_alias=(char*)(to); }
#define MPL__PREV(elt,list) ((char*)((list)->prev))
#define MPL__PREVASGN(elt,list,to) { char **_alias = (char**)&((list)->prev); *_alias=(char*)(to); }
#define MPL__RS(list) { char **_alias = (char**)&(list); *_alias=_tmp; }
#define MPL__CASTASGN(a,b) { char **_alias = (char**)&(a); *_alias=(char*)(b); }
#else 
#define MPL__SV(elt,list)
#define MPL__NEXT(elt,list) ((elt)->next)
#define MPL__NEXTASGN(elt,list,to) ((elt)->next)=(to)
#define MPL__PREV(elt,list) ((elt)->prev)
#define MPL__PREVASGN(elt,list,to) ((elt)->prev)=(to)
#define MPL__RS(list)
#define MPL__CASTASGN(a,b) (a)=(b)
#endif

/******************************************************************************
 * The sort macro is an adaptation of Simon Tatham's O(n log(n)) mergesort    *
 * Unwieldy variable names used here to avoid shadowing passed-in variables.  *
 *****************************************************************************/
#define MPL_LL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  MPL_LDECLTYPE(list) _ls_p;                                                                       \
  MPL_LDECLTYPE(list) _ls_q;                                                                       \
  MPL_LDECLTYPE(list) _ls_e;                                                                       \
  MPL_LDECLTYPE(list) _ls_tail;                                                                    \
  MPL_LDECLTYPE(list) _ls_oldhead;                                                                 \
  MPL_LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      MPL__CASTASGN(_ls_p,list);                                                                   \
      MPL__CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,_ls_e); MPL__RS(list);                     \
          } else {                                                                             \
            MPL__CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,MPL_NULL); MPL__RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

#define MPL_DL_SORT(list, cmp)                                                                     \
do {                                                                                           \
  MPL_LDECLTYPE(list) _ls_p;                                                                       \
  MPL_LDECLTYPE(list) _ls_q;                                                                       \
  MPL_LDECLTYPE(list) _ls_e;                                                                       \
  MPL_LDECLTYPE(list) _ls_tail;                                                                    \
  MPL_LDECLTYPE(list) _ls_oldhead;                                                                 \
  MPL_LDECLTYPE(list) _tmp;                                                                        \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      MPL__CASTASGN(_ls_p,list);                                                                   \
      MPL__CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list);                               \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
          } else {                                                                             \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,_ls_e); MPL__RS(list);                     \
          } else {                                                                             \
            MPL__CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          MPL__SV(_ls_e,list); MPL__PREVASGN(_ls_e,list,_ls_tail); MPL__RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      MPL__CASTASGN(list->prev, _ls_tail);                                                         \
      MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,MPL_NULL); MPL__RS(list);                            \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

#define MPL_CDL_SORT(list, cmp)                                                                    \
do {                                                                                           \
  MPL_LDECLTYPE(list) _ls_p;                                                                       \
  MPL_LDECLTYPE(list) _ls_q;                                                                       \
  MPL_LDECLTYPE(list) _ls_e;                                                                       \
  MPL_LDECLTYPE(list) _ls_tail;                                                                    \
  MPL_LDECLTYPE(list) _ls_oldhead;                                                                 \
  MPL_LDECLTYPE(list) _tmp;                                                                        \
  MPL_LDECLTYPE(list) _tmp2;                                                                       \
  int _ls_insize, _ls_nmerges, _ls_psize, _ls_qsize, _ls_i, _ls_looping;                       \
  if (list) {                                                                                  \
    _ls_insize = 1;                                                                            \
    _ls_looping = 1;                                                                           \
    while (_ls_looping) {                                                                      \
      MPL__CASTASGN(_ls_p,list);                                                                   \
      MPL__CASTASGN(_ls_oldhead,list);                                                             \
      list = NULL;                                                                             \
      _ls_tail = NULL;                                                                         \
      _ls_nmerges = 0;                                                                         \
      while (_ls_p) {                                                                          \
        _ls_nmerges++;                                                                         \
        _ls_q = _ls_p;                                                                         \
        _ls_psize = 0;                                                                         \
        for (_ls_i = 0; _ls_i < _ls_insize; _ls_i++) {                                         \
          _ls_psize++;                                                                         \
          MPL__SV(_ls_q,list);                                                                     \
          if (MPL__NEXT(_ls_q,list) == _ls_oldhead) {                                              \
            _ls_q = NULL;                                                                      \
          } else {                                                                             \
            _ls_q = MPL__NEXT(_ls_q,list);                                                         \
          }                                                                                    \
          MPL__RS(list);                                                                           \
          if (!_ls_q) break;                                                                   \
        }                                                                                      \
        _ls_qsize = _ls_insize;                                                                \
        while (_ls_psize > 0 || (_ls_qsize > 0 && _ls_q)) {                                    \
          if (_ls_psize == 0) {                                                                \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          } else if (_ls_qsize == 0 || !_ls_q) {                                               \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else if (cmp(_ls_p,_ls_q) <= 0) {                                                  \
            _ls_e = _ls_p; MPL__SV(_ls_p,list); _ls_p = MPL__NEXT(_ls_p,list); MPL__RS(list); _ls_psize--; \
            if (_ls_p == _ls_oldhead) { _ls_p = NULL; }                                        \
          } else {                                                                             \
            _ls_e = _ls_q; MPL__SV(_ls_q,list); _ls_q = MPL__NEXT(_ls_q,list); MPL__RS(list); _ls_qsize--; \
            if (_ls_q == _ls_oldhead) { _ls_q = NULL; }                                        \
          }                                                                                    \
          if (_ls_tail) {                                                                      \
            MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,_ls_e); MPL__RS(list);                     \
          } else {                                                                             \
            MPL__CASTASGN(list,_ls_e);                                                             \
          }                                                                                    \
          MPL__SV(_ls_e,list); MPL__PREVASGN(_ls_e,list,_ls_tail); MPL__RS(list);                          \
          _ls_tail = _ls_e;                                                                    \
        }                                                                                      \
        _ls_p = _ls_q;                                                                         \
      }                                                                                        \
      MPL__CASTASGN(list->prev,_ls_tail);                                                          \
      MPL__CASTASGN(_tmp2,list);                                                                   \
      MPL__SV(_ls_tail,list); MPL__NEXTASGN(_ls_tail,list,_tmp2); MPL__RS(list);                           \
      if (_ls_nmerges <= 1) {                                                                  \
        _ls_looping=0;                                                                         \
      }                                                                                        \
      _ls_insize *= 2;                                                                         \
    }                                                                                          \
  } else _tmp=NULL; /* quiet gcc unused variable warning */                                    \
} while (0)

/******************************************************************************
 * singly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define MPL_LL_PREPEND(head,add)                                                                   \
do {                                                                                           \
  (add)->next = head;                                                                          \
  head = add;                                                                                  \
} while (0)

#define MPL_LL_APPEND(head,add)                                                                    \
do {                                                                                           \
  MPL_LDECLTYPE(head) _tmp;                                                                        \
  (add)->next=NULL;                                                                            \
  if (head) {                                                                                  \
    _tmp = head;                                                                               \
    while (_tmp->next) { _tmp = _tmp->next; }                                                  \
    _tmp->next=(add);                                                                          \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
} while (0)

#define MPL_LL_DELETE(head,del)                                                                    \
do {                                                                                           \
  MPL_LDECLTYPE(head) _tmp;                                                                        \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    _tmp = head;                                                                               \
    while (_tmp->next && (_tmp->next != (del))) {                                              \
      _tmp = _tmp->next;                                                                       \
    }                                                                                          \
    if (_tmp->next) {                                                                          \
      _tmp->next = ((del)->next);                                                              \
    }                                                                                          \
  }                                                                                            \
} while (0)

/* Here are VS2008 replacements for MPL_LL_APPEND and MPL_LL_DELETE */
#define MPL_LL_APPEND_VS2008(head,add)                                                             \
do {                                                                                           \
  if (head) {                                                                                  \
    (add)->next = head;     /* use add->next as a temp variable */                             \
    while ((add)->next->next) { (add)->next = (add)->next->next; }                             \
    (add)->next->next=(add);                                                                   \
  } else {                                                                                     \
    (head)=(add);                                                                              \
  }                                                                                            \
  (add)->next=NULL;                                                                            \
} while (0)

#define MPL_LL_DELETE_VS2008(head,del)                                                             \
do {                                                                                           \
  if ((head) == (del)) {                                                                       \
    (head)=(head)->next;                                                                       \
  } else {                                                                                     \
    char *_tmp = (char*)(head);                                                                \
    while (head->next && (head->next != (del))) {                                              \
      head = head->next;                                                                       \
    }                                                                                          \
    if (head->next) {                                                                          \
      head->next = ((del)->next);                                                              \
    }                                                                                          \
    {                                                                                          \
      char **_head_alias = (char**)&(head);                                                    \
      *_head_alias = _tmp;                                                                     \
    }                                                                                          \
  }                                                                                            \
} while (0)
#ifdef MPL_NO_DECLTYPE
#undef MPL_LL_APPEND
#define MPL_LL_APPEND MPL_LL_APPEND_VS2008
#undef MPL_LL_DELETE
#define MPL_LL_DELETE MPL_LL_DELETE_VS2008
#endif
/* end VS2008 replacements */

#define MPL_LL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->next)

#define MPL_LL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)

#define MPL_LL_SEARCH_SCALAR(head,out,field,val)                                                   \
do {                                                                                           \
    MPL_LL_FOREACH(head,out) {                                                                     \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define MPL_LL_SEARCH(head,out,elt,cmp)                                                            \
do {                                                                                           \
    MPL_LL_FOREACH(head,out) {                                                                     \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 

/******************************************************************************
 * doubly linked list macros (non-circular)                                   *
 *****************************************************************************/
#define MPL_DL_PREPEND(head,add)                                                                   \
do {                                                                                           \
 (add)->next = head;                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (head)->prev = (add);                                                                       \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
 }                                                                                             \
 (head) = (add);                                                                               \
} while (0)

#define MPL_DL_APPEND(head,add)                                                                    \
do {                                                                                           \
  if (head) {                                                                                  \
      (add)->prev = (head)->prev;                                                              \
      (head)->prev->next = (add);                                                              \
      (head)->prev = (add);                                                                    \
      (add)->next = NULL;                                                                      \
  } else {                                                                                     \
      (head)=(add);                                                                            \
      (head)->prev = (head);                                                                   \
      (head)->next = NULL;                                                                     \
  }                                                                                            \
} while (0);

#define MPL_DL_DELETE(head,del)                                                                    \
do {                                                                                           \
  if ((del)->prev == (del)) {                                                                  \
      (head)=NULL;                                                                             \
  } else if ((del)==(head)) {                                                                  \
      (del)->next->prev = (del)->prev;                                                         \
      (head) = (del)->next;                                                                    \
  } else {                                                                                     \
      (del)->prev->next = (del)->next;                                                         \
      if ((del)->next) {                                                                       \
          (del)->next->prev = (del)->prev;                                                     \
      } else {                                                                                 \
          (head)->prev = (del)->prev;                                                          \
      }                                                                                        \
  }                                                                                            \
} while (0);


#define MPL_DL_FOREACH(head,el)                                                                    \
    for(el=head;el;el=el->next)

/* this version is safe for deleting the elements during iteration */
#define MPL_DL_FOREACH_SAFE(head,el,tmp)                                                           \
  for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)

/* these are identical to their singly-linked list counterparts */
#define MPL_DL_SEARCH_SCALAR MPL_LL_SEARCH_SCALAR
#define MPL_DL_SEARCH MPL_LL_SEARCH

/******************************************************************************
 * circular doubly linked list macros                                         *
 *****************************************************************************/
#define MPL_CDL_PREPEND(head,add)                                                                  \
do {                                                                                           \
 if (head) {                                                                                   \
   (add)->prev = (head)->prev;                                                                 \
   (add)->next = (head);                                                                       \
   (head)->prev = (add);                                                                       \
   (add)->prev->next = (add);                                                                  \
 } else {                                                                                      \
   (add)->prev = (add);                                                                        \
   (add)->next = (add);                                                                        \
 }                                                                                             \
(head)=(add);                                                                                  \
} while (0)

#define MPL_CDL_DELETE(head,del)                                                                   \
do {                                                                                           \
  if ( ((head)==(del)) && ((head)->next == (head))) {                                          \
      (head) = 0L;                                                                             \
  } else {                                                                                     \
     (del)->next->prev = (del)->prev;                                                          \
     (del)->prev->next = (del)->next;                                                          \
     if ((del) == (head)) (head)=(del)->next;                                                  \
  }                                                                                            \
} while (0);

#define MPL_CDL_FOREACH(head,el)                                                                   \
    for(el=head;el;el=(el->next==head ? 0L : el->next)) 

#define MPL_CDL_FOREACH_SAFE(head,el,tmp1,tmp2)                                                    \
  for((el)=(head), ((tmp1)=(head)?((head)->prev):NULL);                                        \
      (el) && ((tmp2)=(el)->next, 1);                                                          \
      ((el) = (((el)==(tmp1)) ? 0L : (tmp2))))

#define MPL_CDL_SEARCH_SCALAR(head,out,field,val)                                                  \
do {                                                                                           \
    MPL_CDL_FOREACH(head,out) {                                                                    \
      if ((out)->field == (val)) break;                                                        \
    }                                                                                          \
} while(0) 

#define MPL_CDL_SEARCH(head,out,elt,cmp)                                                           \
do {                                                                                           \
    MPL_CDL_FOREACH(head,out) {                                                                    \
      if ((cmp(out,elt))==0) break;                                                            \
    }                                                                                          \
} while(0) 

#endif /* MPL_UTLIST_H */


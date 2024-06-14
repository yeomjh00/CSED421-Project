/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module: edubfm_Hash.c
 *
 * Description:
 *  Some functions are provided to support buffer manager.
 *  Each BfMHashKey is mapping to one table entry in a hash table(hTable),
 *  and each entry has an index which indicates a buffer in a buffer pool.
 *  An ordinary hashing method is used and linear probing strategy is
 *  used if collision has occurred.
 *
 * Exports:
 *  Four edubfm_LookUp(BfMHashKey *, Four)
 *  Four edubfm_Insert(BfMHaskKey *, Two, Four)
 *  Four edubfm_Delete(BfMHashKey *, Four)
 *  Four edubfm_DeleteAll(void)
 */

#include <stdlib.h> /* for malloc & free */

#include "EduBfM_common.h"
#include "EduBfM_Internal.h"

/*@
 * macro definitions
 */

/* Macro: BFM_HASH(k,type)
 * Description: return the hash value of the key given as a parameter
 * Parameters:
 *  BfMHashKey *k   : pointer to the key
 *  Four type       : buffer type
 * Returns: (Two) hash value
 */
#define BFM_HASH(k, type) (((k)->volNo + (k)->pageNo) % HASHTABLESIZE(type))

/*@================================
 * edubfm_Insert()
 *================================*/
/*
 * Function: Four edubfm_Insert(BfMHashKey *, Two, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Insert a new entry into the hash table.
 *  If collision occurs, then use the linear probing method.
 *
 * Returns:
 *  error code
 *    eBADBUFINDEX_BFM - bad index value for buffer table
 */
Four edubfm_Insert(BfMHashKey *key, /* IN a hash key in Buffer Manager */
                   Two index,       /* IN an index used in the buffer pool */
                   Four type)       /* IN buffer type */
{
  Four next;
  Two hashValue;
  CHECKKEY(key); /*@ check validity of key */

  if ((index < 0) || (index > BI_NBUFS(type))) {
    ERR(eBADBUFINDEX_BFM);
    return eBADBUFINDEX_BFM;
  }

  hashValue = BFM_HASH(key, type);

  if (BI_HASHTABLEENTRY(type, hashValue) == NIL) {
    BI_HASHTABLEENTRY(type, hashValue) = index;
    return eNOERROR;
  }

  next = BI_HASHTABLEENTRY(type, hashValue);
  BI_HASHTABLEENTRY(type, hashValue) = index;
  BI_NEXTHASHENTRY(type, index) = next;
  return eNOERROR;
} /* edubfm_Insert */

/*@================================
 * edubfm_Delete()
 *================================*/
/*
 * Function: Four edubfm_Delete(BfMHashKey *, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Look up the entry which corresponds to `key' and
 *  Delete the entry from the hash table.
 *
 * Returns:
 *  error code
 *    eNOTFOUND_BFM - The key isn't in the hash table.
 */
Four edubfm_Delete(BfMHashKey *key, /* IN a hash key in buffer manager */
                   Four type)       /* IN buffer type */
{
  Two cur, prev;
  Two hashValue;

  CHECKKEY(key); /*@ check validity of key */

  hashValue = BFM_HASH(key, type);
  cur = BI_HASHTABLEENTRY(type, hashValue);

  if (cur < 0) {
    ERR(eNOTFOUND_BFM);
    return eNOTFOUND_BFM;
  } else if (BI_NEXTHASHENTRY(type, cur) < 0 &&
             EQUALKEY(&BI_KEY(type, cur),
                      key)) {  // case 1. single value stored & matched
    BI_HASHTABLEENTRY(type, hashValue) = -1;
  }

  while (cur >= 0) {
    prev = cur;
    cur = BI_NEXTHASHENTRY(type, cur);
    if (EQUALKEY(&BI_KEY(type, cur), key)) {
      BI_NEXTHASHENTRY(type, prev) = BI_NEXTHASHENTRY(type, cur);
      return 0;
    }
  }
  return eNOTFOUND_BFM;
} /* edubfm_Delete */

/*@================================
 * edubfm_LookUp()
 *================================*/
/*
 * Function: Four edubfm_LookUp(BfMHashKey *, Four)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Look up the given key in the hash table and return its
 *  corressponding index to the buffer table.
 *
 * Retruns:
 *  index on buffer table entry holding the train specified by 'key'
 *  (NOTFOUND_IN_HTABLE - The key don't exist in the hash table.)
 */
Four edubfm_LookUp(BfMHashKey *key, /* IN a hash key in Buffer Manager */
                   Four type)       /* IN buffer type */
{
  Two i, j; /* indices */
  Two hashValue;

  CHECKKEY(key); /*@ check validity of key */

  hashValue = BFM_HASH(key, type);
  i = BI_HASHTABLEENTRY(type, hashValue);
  // j = BFM_HASH(key, type);

  while (i >= 0) {
    if (EQUALKEY(&BI_KEY(type, i), key)) return i;
    i = BI_NEXTHASHENTRY(type, i);
  }

  return (NOTFOUND_IN_HTABLE);

} /* edubfm_LookUp */

/*@================================
 * edubfm_DeleteAll()
 *================================*/
/*
 * Function: Four edubfm_DeleteAll(void)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Delete all hash entries.
 *
 * Returns:
 *  error code
 */
Four edubfm_DeleteAll(void) {
  Two type,i;

  for(type=0; type < 2; type++){
    for (i = 0; i < HASHTABLESIZE(type); i++) {
      BI_HASHTABLEENTRY(type, i) = NIL;
    }
  }

  return (eNOERROR);

} /* edubfm_DeleteAll() */

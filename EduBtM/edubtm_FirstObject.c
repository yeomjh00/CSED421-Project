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
 * Module: edubtm_FirstObject.c
 *
 * Description : 
 *  Find the first ObjectID of the given Btree. 
 *
 * Exports:
 *  Four edubtm_FirstObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_FirstObject()
 *================================*/
/*
 * Function: Four edubtm_FirstObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*)
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first ObjectID of the given Btree. The 'cursor' will indicate
 *  the first ObjectID in the Btree, and it will be used as successive access
 *  by using the Btree.
 *
 * Returns:
 *  error code
 *    eBADPAGE_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor : A position in the Btree which indicates the first ObjectID.
 *             The first object's object identifier is also returned via this.
 */
Four edubtm_FirstObject(
    PageID  		*root,		/* IN The root of Btree */
    KeyDesc 		*kdesc,		/* IN Btree key descriptor */
    KeyValue 		*stopKval,	/* IN key value of stop condition */
    Four     		stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor 	*cursor)	/* OUT The first ObjectID in the Btree */
{
    int			i;
    Four 		e;		/* error */
    Four 		cmp;		/* result of comparison */
    PageID 		curPid;		/* PageID of the current page */
    PageID 		child;		/* PageID of the child page */
    BtreePage 		*apage;		/* a page pointer */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_LeafEntry 	*lEntry;	/* a leaf entry */
    Two                 alignedKlen;    /* aligned length of the key length */
    Two                 entryLen;
    

    if (root == NULL) ERR(eBADPAGE_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    curPid = *root;
    e = BfM_GetTrain(&curPid, (char**)&apage, PAGE_BUF);
    if (e < 0) ERR(e);

    while (!(apage->any.hdr.type & LEAF)) {
        MAKE_PAGEID(child, curPid.volNo, apage->bi.hdr.p0);

        e = BfM_FreeTrain(&curPid, PAGE_BUF);
        if (e < 0) ERR(e);

        e = BfM_GetTrain(&child, (char**)&apage, PAGE_BUF);
        if (e < 0) ERR(e);
        
        curPid = child;
    }

    lEntry = (btm_LeafEntry*)&(apage->bl.data[apage->bl.slot[0]]);
    alignedKlen = ALIGNED_LENGTH(lEntry->klen);
    
    cursor->oid = *(ObjectID*)&(lEntry->kval[ALIGNED_LENGTH(lEntry->klen)]);
    cursor->key.len = lEntry->klen;
    memcpy(cursor->key.val, lEntry->kval, lEntry->klen);
    cursor->leaf = curPid;
    cursor->slotNo = 0;
    cursor->flag = (One)CURSOR_ON;
    cmp = edubtm_KeyCompare(kdesc, stopKval, &cursor->key);
    switch (cmp){
        case GREATER:
            cursor->flag = (One)CURSOR_ON;
            break;

        case LESS:
            cursor->flag = (One)CURSOR_EOS;
            break;

        case EQUAL:
            cursor->flag = (stopCompOp == SM_LT) ? (One)CURSOR_EOS : (One)CURSOR_ON;
            break;

        default:
            ERR(eBADCOMPOP_BTM);
    }

    
    e = BfM_FreeTrain(&curPid, PAGE_BUF);
    if (e < 0) ERR(e);
    

    return(eNOERROR);
    
} /* edubtm_FirstObject() */

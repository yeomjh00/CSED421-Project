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
 * Module: EduBtM_FetchNext.c
 *
 * Description:
 *  Find the next ObjectID satisfying the given condition. The current ObjectID
 *  is specified by the 'current'.
 *
 * Exports:
 *  Four EduBtM_FetchNext(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"


/*@ Internal Function Prototypes */
Four edubtm_FetchNext(KeyDesc*, KeyValue*, Four, BtreeCursor*, BtreeCursor*);



/*@================================
 * EduBtM_FetchNext()
 *================================*/
/*
 * Function: Four EduBtM_FetchNext(PageID*, KeyDesc*, KeyValue*,
 *                              Four, BtreeCursor*, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Fetch the next ObjectID satisfying the given condition.
 * By the B+ tree structure modification resulted from the splitting or merging
 * the current cursor may point to the invalid position. So we should adjust
 * the B+ tree cursor before using the cursor.
 *
 * Returns:
 *  error code
 *    eBADPARAMETER_BTM
 *    eBADCURSOR
 *    some errors caused by function calls
 */
Four EduBtM_FetchNext(
    PageID                      *root,          /* IN root page's PageID */
    KeyDesc                     *kdesc,         /* IN key descriptor */
    KeyValue                    *kval,          /* IN key value of stop condition */
    Four                        compOp,         /* IN comparison operator of stop condition */
    BtreeCursor                 *current,       /* IN current B+ tree cursor */
    BtreeCursor                 *next)          /* OUT next B+ tree cursor */
{
    int							i;
    Four                        e;              /* error number */
    Four                        cmp;            /* comparison result */
    Two                         slotNo;         /* slot no. of a leaf page */
    Two                         oidArrayElemNo; /* element no. of the array of ObjectIDs */
    Two                         alignedKlen;    /* aligned length of key length */
    PageID                      overflow;       /* temporary PageID of an overflow page */
    Boolean                     found;          /* search result */
    ObjectID                    *oidArray;      /* array of ObjectIDs */
    BtreeLeaf                   *apage;         /* pointer to a buffer holding a leaf page */
    BtreeOverflow               *opage;         /* pointer to a buffer holding an overflow page */
    btm_LeafEntry               *entry;         /* pointer to a leaf entry */
    BtreeCursor                 tCursor;        /* a temporary Btree cursor */
  
    
    /*@ check parameter */
    if (root == NULL || kdesc == NULL || kval == NULL || current == NULL || next == NULL)
	ERR(eBADPARAMETER_BTM);
    
    /* Is the current cursor valid? */
    if (current->flag != CURSOR_ON && current->flag != CURSOR_EOS)
		ERR(eBADCURSOR);
    
    if (current->flag == CURSOR_EOS) return(eNOERROR);
    
    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    e = edubtm_FetchNext(kdesc, kval, compOp, current, next);
    if (e < 0) ERR(e);
    
    return(eNOERROR);
    
} /* EduBtM_FetchNext() */



/*@================================
 * edubtm_FetchNext()
 *================================*/
/*
 * Function: Four edubtm_FetchNext(KeyDesc*, KeyValue*, Four,
 *                              BtreeCursor*, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Get the next item. We assume that the current cursor is valid; that is.
 *  'current' rightly points to an existing ObjectID.
 *
 * Returns:
 *  Error code
 *    eBADCOMPOP_BTM
 *    some errors caused by function calls
 */
Four edubtm_FetchNext(
    KeyDesc  		*kdesc,		/* IN key descriptor */
    KeyValue 		*kval,		/* IN key value of stop condition */
    Four     		compOp,		/* IN comparison operator of stop condition */
    BtreeCursor 	*current,	/* IN current cursor */
    BtreeCursor 	*next)		/* OUT next cursor */
{
    Four 		e;		/* error number */
    Four 		cmp;		/* comparison result */
    Two 		alignedKlen;	/* aligned length of a key length */
    PageID 		leaf;		/* temporary PageID of a leaf page */
    PageID 		overflow;	/* temporary PageID of an overflow page */
    ObjectID 		*oidArray;	/* array of ObjectIDs */
    BtreeLeaf 		*apage;		/* pointer to a buffer holding a leaf page */
    BtreeOverflow 	*opage;		/* pointer to a buffer holding an overflow page */
    btm_LeafEntry 	*entry;		/* pointer to a leaf entry */
    Boolean invalidCondition;
    
    
    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    leaf = current->leaf;
    overflow = current->leaf;
    next->flag = (One)CURSOR_ON;
    next->slotNo = current->slotNo;

    e = BfM_GetTrain(&leaf, (char**)&apage, PAGE_BUF);
    if(e < 0) ERR(e);

    if (compOp == SM_LT || compOp == SM_LE || compOp == SM_EQ || compOp == SM_EOF){
        next->slotNo = current->slotNo + 1;
    } else if (compOp == SM_GT || compOp == SM_GE || compOp == SM_BOF){
        next->slotNo = current->slotNo - 1;
    }

    if (next->slotNo < 0){
        if (apage->hdr.prevPage == NIL){
            next->flag = CURSOR_EOS;
        } else {
            
            e = BfM_FreeTrain(&leaf, PAGE_BUF);
            if(e < 0) ERR(e);

            MAKE_PAGEID(overflow, leaf.volNo, apage->hdr.prevPage);
            e = BfM_GetTrain(&overflow, (char**)&apage, PAGE_BUF);
            if(e < 0) ERR(e);

            next->slotNo = apage->hdr.nSlots - 1;
        }
    } else if (next->slotNo >= apage->hdr.nSlots){
        if (apage->hdr.nextPage == NIL){
            next->flag = CURSOR_EOS;
        } else{

            e = BfM_FreeTrain(&leaf, PAGE_BUF);
            if(e < 0) ERR(e);

            MAKE_PAGEID(overflow, leaf.volNo, apage->hdr.nextPage);
            e = BfM_GetTrain(&overflow, (char**)&apage, PAGE_BUF);
            if(e < 0) ERR(e);

            next->slotNo = 0;
        }
    }

    next->leaf = overflow;

    entry = &apage->data[apage->slot[-next->slotNo]];
    alignedKlen = ALIGNED_LENGTH(entry->klen);

    memcpy(&next->key, &entry->klen, sizeof(KeyValue));
    next->oid = *(ObjectID*)&entry->kval[alignedKlen];

    
    invalidCondition = FALSE;
    cmp = edubtm_KeyCompare(kdesc, &next->key, kval);

    switch(compOp){
        case SM_EQ:
            invalidCondition = cmp != EQUAL;
            break;
        case SM_LT:
            invalidCondition = cmp != LESS;
            break;
        case SM_LE:
            invalidCondition = (cmp == GREATER);
            break;
        case SM_GT:
            invalidCondition = cmp != GREATER;
            break;
        case SM_GE:
            invalidCondition = (cmp == LESS);
            break;
        default:
            break;
    }

    if (invalidCondition){
        next->flag = CURSOR_EOS;
    }

    e = BfM_FreeTrain(&overflow, PAGE_BUF);
    if (e < 0) ERR(e);
    
    return(eNOERROR);
    
} /* edubtm_FetchNext() */

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
 * Module: edubtm_Insert.c
 *
 * Description : 
 *  This function edubtm_Insert(...) recursively calls itself until the type
 *  of a root page becomes LEAF.  If the given root page is an internal,
 *  it recursively calls itself using a proper child.  If the result of
 *  the call occur spliting, merging, or redistributing the children, it
 *  may insert, delete, or replace its own internal item, and if the given
 *  root page may be merged, splitted, or redistributed, it affects the
 *  return values.
 *
 * Exports:
 *  Four edubtm_Insert(ObjectID*, PageID*, KeyDesc*, KeyValue*, ObjectID*,
 *                  Boolean*, Boolean*, InternalItem*, Pool*, DeallocListElem*)
 *  Four edubtm_InsertLeaf(ObjectID*, PageID*, BtreeLeaf*, KeyDesc*, KeyValue*,
 *                      ObjectID*, Boolean*, Boolean*, InternalItem*)
 *  Four edubtm_InsertInternal(ObjectID*, BtreeInternal*, InternalItem*,
 *                          Two, Boolean*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "OM_Internal.h"	/* for SlottedPage containing catalog object */
#include "EduBtM_Internal.h"
#include <assert.h>


/*@================================
 * edubtm_Insert()
 *================================*/
/*
 * Function: Four edubtm_Insert(ObjectID*, PageID*, KeyDesc*, KeyValue*,
 *                           ObjectID*, Boolean*, Boolean*, InternalItem*,
 *                           Pool*, DeallocListElem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  If the given root is a leaf page, it should get the correct entry in the
 *  leaf. If the entry is already in the leaf, it simply insert it into the
 *  entry and increment the number of ObjectIDs.  If it is not in the leaf it
 *  makes a new entry and insert it into the leaf.
 *  If there is not enough spage in the leaf, the page should be splitted.  The
 *  overflow page may be used or created by this routine. It is created when
 *  the size of the entry is greater than a third of a page.
 * 
 *  'h' is TRUE if the given root page is splitted and the entry item will be
 *  inserted into the parent page.  'f' is TRUE if the given page is not half
 *  full because of creating a new overflow page.
 *
 * Returns:
 *  Error code
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 */

Four edubtm_Insert(
    ObjectID                    *catObjForFile,         /* IN catalog object of B+-tree file */
    PageID                      *root,                  /* IN the root of a Btree */
    KeyDesc                     *kdesc,                 /* IN Btree key descriptor */
    KeyValue                    *kval,                  /* IN key value */
    ObjectID                    *oid,                   /* IN ObjectID which will be inserted */
    Boolean                     *f,                     /* OUT whether it is merged by creating a new overflow page */
    Boolean                     *h,                     /* OUT whether it is splitted */
    InternalItem                *item,                  /* OUT Internal Item which will be inserted */
                                                        /*     into its parent when 'h' is TRUE */
    Pool                        *dlPool,                /* INOUT pool of dealloc list */
    DeallocListElem             *dlHead)                /* INOUT head of the dealloc list */
{
    Four                        e;                      /* error number */
    Boolean                     lh;                     /* local 'h' */
    Boolean                     lf;                     /* local 'f' */
    Two                         idx;                    /* index for the given key value */
    PageID                      newPid;                 /* a new PageID */
    KeyValue                    tKey;                   /* a temporary key */
    InternalItem                litem;                  /* a local internal item */
    BtreePage                   *apage;                 /* a pointer to the root page */
    btm_InternalEntry           *iEntry;                /* an internal entry */
    Two                         iEntryOffset;           /* starting offset of an internal entry */
    SlottedPage                 *catPage;               /* buffer page containing the catalog object */
    sm_CatOverlayForBtree       *catEntry;              /* pointer to Btree file catalog information */
    PhysicalFileID              pFid;                   /* B+-tree file's FileID */
    Boolean                     found;


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    *f = *h = FALSE;
    
    e = BfM_GetTrain(root, (char**)&apage, PAGE_BUF);
    if (e < 0) ERR(e);

    if(apage->any.hdr.type & INTERNAL){
        // next page determined
        found = edubtm_BinarySearchInternal(apage, kdesc, kval, &idx);

        if(idx == -1){
            MAKE_PAGEID(newPid, root->volNo, apage->bi.hdr.p0);
        }
        else {
            iEntryOffset = apage->bi.slot[-idx];
            iEntry = (btm_InternalEntry*)&apage->bi.data[iEntryOffset];
            MAKE_PAGEID(newPid, root->volNo, iEntry->spid);
        }
        lf = lh = FALSE;

        e = edubtm_Insert(catObjForFile, &newPid, kdesc, kval, oid, &lf, &lh, &litem, dlPool, dlHead);
        if (e < 0) ERR(e);

        if(lh){
            tKey.len = litem.klen;
            memcpy(tKey.val, litem.kval, litem.klen);

            found = edubtm_BinarySearchInternal(&(apage->bi), kdesc, &tKey, &idx);

            e = edubtm_InsertInternal(catObjForFile, &(apage->bi), &litem, idx, h, item);
            if (e < 0) ERR(e);

            e = BfM_SetDirty(root, PAGE_BUF);
            if (e < 0) ERR(e);
        }

    } else if(apage->any.hdr.type & LEAF){
        e = edubtm_InsertLeaf(catObjForFile, root, &(apage->bl), kdesc, kval, oid, f, h, item);
        if (e < 0) ERR(e);

        e = BfM_SetDirty(root, PAGE_BUF);
        if (e < 0) ERR(e);
    }

    e = BfM_FreeTrain(root, PAGE_BUF);
    if(e < 0) ERR(e);
    
    return(eNOERROR);
    
}   /* edubtm_Insert() */



/*@================================
 * edubtm_InsertLeaf()
 *================================*/
/*
 * Function: Four edubtm_InsertLeaf(ObjectID*, PageID*, BtreeLeaf*, KeyDesc*,
 *                               KeyValue*, ObjectID*, Boolean*, Boolean*,
 *                               InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Insert into the given leaf page an ObjectID with the given key.
 *
 * Returns:
 *  Error code
 *    eDUPLICATEDKEY_BTM
 *    eDUPLICATEDOBJECTID_BTM
 *    some errors causd by function calls
 *
 * Side effects:
 *  1) f : TRUE if the leaf page is underflowed by creating an overflow page
 *  2) h : TRUE if the leaf page is splitted by inserting the given ObjectID
 *  3) item : item to be inserted into the parent
 */
Four edubtm_InsertLeaf(
    ObjectID                    *catObjForFile, /* IN catalog object of B+-tree file */
    PageID                      *pid,           /* IN PageID of Leag Page */
    BtreeLeaf                   *page,          /* INOUT pointer to buffer page of Leaf page */
    KeyDesc                     *kdesc,         /* IN Btree key descriptor */
    KeyValue                    *kval,          /* IN key value */
    ObjectID                    *oid,           /* IN ObjectID which will be inserted */
    Boolean                     *f,             /* OUT whether it is merged by creating */
                                                /*     a new overflow page */
    Boolean                     *h,             /* OUT whether it is splitted */
    InternalItem                *item)          /* OUT Internal Item which will be inserted */
                                                /*     into its parent when 'h' is TRUE */
{
    Four                        e;              /* error number */
    Two                         i;
    Two                         idx;            /* index for the given key value */
    LeafItem                    leaf;           /* a Leaf Item */
    Boolean                     found;          /* search result */
    btm_LeafEntry               *entry;         /* an entry in a leaf page */
    Two                         entryOffset;    /* start position of an entry */
    Two                         alignedKlen;    /* aligned length of the key length */
    PageID                      ovPid;          /* PageID of an overflow page */
    Two                         entryLen;       /* length of an entry */
    ObjectID                    *oidArray;      /* an array of ObjectIDs */
    Two                         oidArrayElemNo; /* an index for the ObjectID array */
    Two                         neededSpace;


    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    
    /*@ Initially the flags are FALSE */
    *h = *f = FALSE;

    // edubtm_SplitLeaf(), edubtm_CompactLeafPage(), edubtm_BinarySearchLeaf()
    found = edubtm_BinarySearchLeaf(page, kdesc, kval, &idx);
    if (found == TRUE) ERR(eDUPLICATEDKEY_BTM);

    alignedKlen = (kdesc->kpart[0].type == SM_INT) ? \
        ALIGNED_LENGTH(kdesc->kpart[0].length) : ALIGNED_LENGTH(kval->len);

    entryLen = sizeof(Two) + sizeof(Two) + alignedKlen + sizeof(ObjectID); // entry + objectID + slot
    neededSpace = entryLen + sizeof(Two);

    if (BL_FREE(page) >= neededSpace){
        if (BL_CFREE(page) < neededSpace){
            edubtm_CompactLeafPage(page, NIL); 
        }

        for(i = page->hdr.nSlots - 1; i > idx; i--){
            page->slot[-(i+1)] = page->slot[-i];
        }

        entryOffset = page->hdr.free;
        page->slot[-(idx+1)] = entryOffset;

        entry = (btm_LeafEntry*)&page->data[entryOffset];
        entry->nObjects = 1;
        entry->klen = kval->len;

        memcpy(entry->kval, kval->val, alignedKlen); // @todo, string-case.
        memcpy(&entry->kval[alignedKlen], oid, sizeof(ObjectID));

        page->hdr.free = page->hdr.free + entryLen;
        page->hdr.nSlots = page->hdr.nSlots + 1;
    } else{
        *h = TRUE;
        leaf.nObjects = 1;
        leaf.oid = *oid;
        memcpy(&leaf.klen, kval, sizeof(KeyValue));
        e = edubtm_SplitLeaf(catObjForFile, pid, page, idx, &leaf, item);
        if (e < 0) ERR(e);
    }    


    return(eNOERROR);
    
} /* edubtm_InsertLeaf() */



/*@================================
 * edubtm_InsertInternal()
 *================================*/
/*
 * Function: Four edubtm_InsertInternal(ObjectID*, BtreeInternal*, InternalItem*, Two, Boolean*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  This routine insert the given internal item into the given page. If there
 *  is not enough space in the page, it should split the page and the new
 *  internal item should be returned for inserting into the parent.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 *
 * Side effects:
 *  h:	TRUE if the page is splitted
 *  ritem: an internal item which will be inserted into parent
 *          if spliting occurs.
 */
Four edubtm_InsertInternal(
    ObjectID            *catObjForFile, /* IN catalog object of B+-tree file */
    BtreeInternal       *page,          /* INOUT Page Pointer */
    InternalItem        *item,          /* IN Iternal item which is inserted */
    Two                 high,           /* IN index in the given page */
    Boolean             *h,             /* OUT whether the given page is splitted */
    InternalItem        *ritem)         /* OUT if the given page is splitted, the internal item may be returned by 'ritem'. */
{
    Four                e;              /* error number */
    Two                 i;              /* index */
    Two                 entryOffset;    /* starting offset of an internal entry */
    Two                 entryLen;       /* length of the new entry */
    btm_InternalEntry   *entry;         /* an internal entry of an internal page */
    Two                 neededSpace;

    
    /*@ Initially the flag are FALSE */
    *h = FALSE;
    entryLen = sizeof(ShortPageID) + ALIGNED_LENGTH(item->klen + sizeof(Two));
    neededSpace = entryLen + sizeof(SlotNo);

    if (BI_FREE(page) >= neededSpace){
        if (BI_CFREE(page) < neededSpace){
            edubtm_CompactInternalPage(page, NIL);
        }
        entryOffset = page->hdr.free;

        for (i = page->hdr.nSlots - 1; i > high; i--){
            page->slot[-(i+1)] = page->slot[-i];
        }
        page->slot[-(high + 1)] = entryOffset;
        entry = (btm_InternalEntry*)&page->data[entryOffset];
        entry->spid = item->spid;
        entry->klen = item->klen;
        memcpy(entry->kval, item->kval, item->klen);
        page->hdr.free = page->hdr.free + entryLen;
        page->hdr.nSlots ++;
    } else{
        e = edubtm_SplitInternal(catObjForFile, page, high, item, ritem);
        if (e < 0) ERR(e);
        *h = TRUE;
    }
    
    return(eNOERROR);
    
} /* edubtm_InsertInternal() */


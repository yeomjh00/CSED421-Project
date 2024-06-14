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
 * Module: edubtm_Compare.c
 *
 * Description : 
 *  This file includes two compare routines, one for keys used in Btree Index
 *  and another for ObjectIDs.
 *
 * Exports: 
 *  Four edubtm_KeyCompare(KeyDesc*, KeyValue*, KeyValue*)
 *  Four edubtm_ObjectIdComp(ObjectID*, ObjectID*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_KeyCompare()
 *================================*/
/*
 * Function: Four edubtm_KeyCompare(KeyDesc*, KeyValue*, KeyValue*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Compare key1 with key2.
 *  key1 and key2 are described by the given parameter "kdesc".
 *
 * Returns:
 *  result of omparison (positive numbers)
 *    EQUAL : key1 and key2 are same
 *    GREAT : key1 is greater than key2
 *    LESS  : key1 is less than key2
 *
 * Note:
 *  We assume that the input data are all valid.
 *  User should check the KeyDesc is valid.
 */
Four edubtm_KeyCompare(
    KeyDesc                     *kdesc,		/* IN key descriptor for key1 and key2 */
    KeyValue                    *key1,		/* IN the first key value */
    KeyValue                    *key2)		/* IN the second key value */
{
    register unsigned char      *left;          /* left key value */
    register unsigned char      *right;         /* right key value */
    Two                         i;              /* index for # of key parts */
    Two                         j;              /* temporary variable */
    Two                         kpartSize;      /* size of the current kpart */
    Two                         len1, len2;	/* string length */
    Two_Invariable              s1, s2;         /* 2-byte short values */
    Four_Invariable             i1, i2;         /* 4-byte int values */
    Four_Invariable             l1, l2;         /* 4-byte long values */
    Eight_Invariable            ll1, ll2;       /* 8-byte long long values */
    float                       f1, f2;         /* float values */
    double                      d1, d2;		/* double values */
    PageID                      pid1, pid2;	/* PageID values */
    OID                         oid1, oid2;     /* OID values */
    Two                         ind;
    Two                         maxIter;
    Two                         keyOffset1;
    Two                         keyOffset2;
    

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    keyOffset1 = 0;
    keyOffset2 = 0;
    maxIter = (kdesc->flag & KEYFLAG_UNIQUE) ? 1 : kdesc->nparts;
    for(i = 0; i < maxIter; i++){
        ind = 1;
        switch(kdesc->kpart[i].type){
            case SM_INT:
                i1 = *(Four_Invariable*)&key1->val[keyOffset1];
                i2 = *(Four_Invariable*)&key2->val[keyOffset2];
                if(i1 < i2){
                    return LESS;
                } else if (i1 > i2){
                    return GREATER;
                }
                keyOffset1 += 4;
                keyOffset2 += 4;
                break;
            case SM_VARSTRING:
                len1 = *(Two*)&key1->val[keyOffset1];
                len2 = *(Two*)&key2->val[keyOffset2];

                for(i = keyOffset1 + 2, j = keyOffset2 + 2; i < len1, j < len2; i++, j++){
                    if (key1->val[i] == 0 && key2->val[j] == 0){
                        ind = 0;
                        break;
                    } else if(key1->val[i] < key2->val[j]){
                        return LESS;
                    } else if (key1->val[i] > key2->val[j]){
                        return GREATER;
                    }
                }

                if (ind && len1 != len2){
                    if (len1 > len2){
                        return GREATER;
                    } else{
                        return LESS;
                    }
                }
                keyOffset1 += len1;
                keyOffset2 += len2;
                break;
            default:
                ERR(eNOTSUPPORTED_EDUBTM);
        }
    }

    return(EQUAL); 
    
}   /* edubtm_KeyCompare() */

/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file history.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include <history.h>


#ifndef _history_c_
#define _history_c_

void initializeHistory(history** storia, int size){
    int err=0;
    *storia=malloc(sizeof(history));
    MEMORYCHECK(*storia);
    (*storia)->data=malloc(size*sizeof(message_t*));
    MEMORYCHEK((*storia)->data);
    memset((*storia)->data,NULL,size);
    history->size=size;
    history->first=0;
    history->last=size-1;
    return 0;
}

int addMessage(history* storia, message_t* mex){
    if(mex==NULL || storia==NULL) return -1;
    int i=(history->last+1)%history->size;
    if(history->data[i]!=NULL){
        free(history->data[i]);
        history->first=i+1;
    }
    history->data[i]=mex;
    history->last=i;
    return 0;
}

void freeHistory(history* storia){
    if(storia!=NULL){
        free(storia->data);
        free(storia);
    }
}

#endif //_history_c_

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
    memset((*storia)->data,NULL,size*sizeof(char));
    (*storia)->pending=malloc(size*sizeof(int));
    MEMORYCHECK((*storia)->pending);
    memset((*storia)->pending,0,size*sizeof(int));
    history->size=size;
    history->first=0;
    history->last=size-1;
    return 0;
}

int addMessage(history* storia, message_t* mex,int fd){
    if(mex==NULL || storia==NULL) return -1;
    int i=(storia->last+1)%history->size;
    if(storia->data[i]!=NULL){
        free(storia->data[i]);
        storia->first=i+1;
    }
    if(fd==-1) storia->pending[i]=1
    else storia->pending[i]=0
    storia->data[i]=mex;
    storia->last=i;
    return 0;
}

void freeHistory(history* storia){
    if(storia!=NULL){
        free(storia->data);
        free(storia->pending);
        free(storia);
    }
}

#endif //_history_c_

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
#include <config.h>
#include <stdlib.h>


#ifndef _history_c_
#define _history_c_

void initializeHistory(history** storia, int size){
    *storia=malloc(sizeof(history));
    MEMORYCHECK(*storia);
    memset(*storia,0,sizeof(history));
    (*storia)->data=malloc(size*sizeof(message_t*));
    MEMORYCHECK((*storia)->data);
    memset((*storia)->data,0,size*sizeof(message_t*));
    (*storia)->pending=malloc(size*sizeof(int));
    MEMORYCHECK((*storia)->pending);
    memset((*storia)->pending,0,size*sizeof(int));
    (*storia)->size=size;
    (*storia)->first=0;
    (*storia)->last=size-1;
    (*storia)->nel=0;
}

int addMessage(history* storia, message_t* mex,int fd){
    if(mex==NULL || storia==NULL) return -1;
    int i=(storia->last+1)%storia->size;
    if(storia->data[i]!=NULL){
        free(storia->data[i]->data.buf);
        free(storia->data[i]);
        storia->first=i+1;
    }
    if(storia->nel!=storia->size)++storia->nel;
    if(fd==-1) storia->pending[i]=-1;
    else storia->pending[i]=0;
    storia->data[i]=mex;
    storia->last=i;
    return 0;
}

history* copyHistory(history* storia){
    history* newHistory = NULL;
    if(storia){
        initializeHistory(&newHistory, storia->size);
        int i=storia->first;
        int j=0;
        int el=0;
        while(el<storia->nel){
            if(storia->data[i]){
                newHistory->data[j]=copymex(*storia->data[i]);
                newHistory->pending[j]=storia->pending[i];
                newHistory->last=j;
                j=(j+1)%newHistory->size;
                ++(newHistory->nel);
            }
            i=(i+1)%newHistory->size;
            ++el;
        }
    }

    return newHistory;
}

void resetPending(history* storia){
    memset(storia->pending,0,(storia->size)*sizeof(int));
}

void freeHistory(history* storia){
    if(storia!=NULL){
        int i=storia->first;
        int el=0;
        while(el<storia->nel){
            if(storia->data[i]){
                free(storia->data[i]->data.buf);
                free(storia->data[i]);
            }
            i=(i+1)%storia->size;
            ++el;
        }
        free(storia->data);
        free(storia->pending);
        free(storia);
    }
}

#endif //_history_c_

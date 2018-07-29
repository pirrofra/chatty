/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file queue.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/


#include <queue.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <config.h>

#ifndef _queue_c_
#define _queue_c_

int initializeQueue(queue** coda, int size){
    int err=0;
    *coda=malloc(sizeof(queue));
    MEMORYCHECK(*coda);
    (*coda)->Table=calloc(size,sizeof(int));
    MEMORYCHECK((*coda)->Table);
    (*coda)->size=size;
    (*coda)->first=0;
    (*coda)->last=size-1;
    (*coda)->nelem=0;
    SYSCALLCHECK(pthread_mutex_init(&((*coda)->lock),NULL),"Inizializzazione Mutex Lock");
    SYSCALLCHECK(pthread_cond_init(&((*coda)->full),NULL),"Inizializzazione Variabile di Condizionamento");
    SYSCALLCHECK(err=pthread_cond_init(&((*coda)->empty),NULL),"Inizializzazione Variabile di Condizionamento");
    return 0;

}

int enqueue(queue* coda, int fd){
    int i=0;
    SYSCALLCHECK(pthread_mutex_lock(&(coda->lock)), "Mutex Lock");
    while(coda->nelem==coda->size){
        SYSCALLCHECK(pthread_cond_wait(&(coda->full),&(coda->lock)),"Wait su V.C.");
    }
    i=(coda->last+1)%coda->size;
    coda->Table[i]=fd;
    coda->last=i;
    ++coda->nelem;
    SYSCALLCHECK(pthread_cond_signal(&(coda->empty)),"Signal su Variabile di Condizionamento");
    SYSCALLCHECK(pthread_mutex_unlock(&(coda->lock)),"Rilascio del Mutex Lock");
    return 0;
}

int dequeue(queue*coda, int* fd){
    int err=0;
    SYSCALLCHECK(pthread_mutex_lock(&(coda->lock),"Acquisizione del Mutex Lock");
    while(coda->nelem==0){
        SYSCALLCHECK(pthread_cond_wait(&(coda->empty),&(coda->lock)),"Wait su Variabile di Condizionamento");
    }
    *fd=coda->Table[coda->first];
    coda->first=(coda->first+1)%coda->size;
    --coda->nelem;
    SYSCALLCHECK(pthread_cond_signal(&(coda->full)),"Signal su Variabile di Condizionamento");
    SYSCALLCHECK(pthread_mutex_unlock(&(coda->lock)),"Rilascio del Mutex Lock");
    return 0;
}

int freeQueue(queue* coda){
    int err=0;
    free(coda->Table);
    SYSCALLCHECK(pthread_mutex_destroy(&(coda->lock)),"Cancellazione del Mutex Lock");
    SYSCALLCHECK(pthread_cond_destroy(&(coda->empty)), "Cancellazione della Variabile di Condizionamento");
    SYSCALLCHECK(pthread_cond_destroy(&(coda->full)), "Cancellazione della Variabile di Condizionamento");
    free(coda);
    return 0;
}

#endif //_queue_c_
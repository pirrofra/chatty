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

/**
 * @function initializeQueue
 * @brief funzione che inizializza la queue
 * @param coda puntatore alla coda da inizializzare;
 * @param size dimensione della coda
 * @return 0 successo, -1 errore
*/
int initializeQueue(queue* coda, int size){
    (coda)->Table=calloc(size,sizeof(int));
    MEMORYCHECK((coda)->Table);
    //alloca l'array per la coda
    (coda)->size=size;
    (coda)->first=0;
    (coda)->last=size-1;
    (coda)->nelem=0;
    (coda)->forceWakeUp=0;
    SYSCALLCHECK(pthread_mutex_init(&((coda)->lock),NULL),"Inizializzazione Mutex Lock");
    SYSCALLCHECK(pthread_cond_init(&((coda)->full),NULL),"Inizializzazione Variabile di Condizionamento");
    SYSCALLCHECK(pthread_cond_init(&((coda)->empty),NULL),"Inizializzazione Variabile di Condizionamento");
    //inizializza Mutex Lock e Variabili di Condizionamento
    return 0;

}

/**
 * @function enqueue
 * @brief aggiunge un file descriptor alla coda
 * @param coda puntatore alla coda in cui inserire l'elemento
 * @param fd file descriptor da aggiungere alla coda
 * @returns 0 successo, -1 errore
*/
int enqueue(queue* coda, int fd){
    int i=0;
    if(coda==NULL||fd<0) return -1;
    SYSCALLCHECK(pthread_mutex_lock(&(coda->lock)), "Mutex Lock"); //Acquisizione Mutex Lock
    while(coda->nelem==coda->size){
        SYSCALLCHECK(pthread_cond_wait(&(coda->full),&(coda->lock)),"Wait su V.C."); //Wait se la coda è piena
        if(coda->forceWakeUp==1) {
            if((errno=pthread_mutex_unlock(&coda->lock))) perror("Mutex Lock");
            return -1;
            //wake up forzato, enqueue fallisce
        }
    }
    i=(coda->last+1)%coda->size;
    coda->Table[i]=fd;
    coda->last=i;
    ++coda->nelem;
    //aggiungo l'elemento in coda
    SYSCALLCHECK(pthread_cond_signal(&(coda->empty)),"Signal su Variabile di Condizionamento");
    SYSCALLCHECK(pthread_mutex_unlock(&(coda->lock)),"Rilascio del Mutex Lock");
    //signal su chi aspettava che la coda si riempisse, rilascio della mutex
    return 0;
}

/**
 * @function dequeue
 * @brief prende il primo elemento della coda e lo restituisce. Se la coda è vuota si mette in attesa.
 * @param coda puntatore alla coda da cui prendere l'elemento.
 * @param fd puntatore al file descriptor estratto.
 * @return 0 successo, -1 errore.
*/
int dequeue(queue*coda, int* fd){
    if(coda==NULL) return -1;
    SYSCALLCHECK(pthread_mutex_lock(&(coda->lock)),"Acquisizione del Mutex Lock"); //Acquisizione Mutex Lock
    while(coda->nelem==0){
        SYSCALLCHECK(pthread_cond_wait(&(coda->empty),&(coda->lock)),"Wait su Variabile di Condizionamento"); //wait se la coda è vuota
        if(coda->forceWakeUp==1) {
            if((errno=pthread_mutex_unlock(&coda->lock))) perror("Mutex Lock");
            return -1;
            //wake up forzato, dequeue fallisce
        }
    }
    *fd=coda->Table[coda->first];
    coda->first=(coda->first+1)%coda->size;
    --coda->nelem;
    //elimino un elemento dalla coda
    SYSCALLCHECK(pthread_cond_signal(&(coda->full)),"Signal su Variabile di Condizionamento");
    SYSCALLCHECK(pthread_mutex_unlock(&(coda->lock)),"Rilascio del Mutex Lock");
    //signal su chi aspettava che la coda liberasse un posto, rilascio mutex
    return 0;
}

/**
 * @function forceWakeUp
 * @brief sveglia tutti i thread in wait e fa fallire le loro enqueue e dequeue
 * @param coda puntatore alla coda da svegliare
*/
void forceWakeEveryone(queue*coda){
    coda->forceWakeUp=1;
    if((errno=pthread_cond_broadcast(&coda->empty))) perror("BroadCast Variabile Condizionamento");
    if((errno=pthread_cond_broadcast(&coda->full))) perror("BroadCast Variabile Condizionamento");
    //setta la variabile forceWakeUp a 1 e fa broadcast su entrambe le variabili di condizionamento
}

/**
 * @function freeQueue
 * @brief libera lo spazio occupato per la gestione della coda
 * @param coda puntatore alla coda da liberare
 * @return 0 successo, -1 errore
*/
void freeQueue(queue* coda){
    if(coda!=NULL){
    free(coda->Table);
    //libero array della coda
    if((errno=pthread_cond_broadcast(&(coda->empty)))) perror("Broadcast di Condizionamento");
    if((errno=pthread_cond_destroy(&(coda->empty)))) perror("Cancellazione della Variabile di Condizionamento");
    if((errno=pthread_cond_broadcast(&(coda->full)))) perror("Broadcast di Condizionamento");
    if((errno=pthread_cond_destroy(&(coda->full)))) perror("Cancellazione della Variabile di Condizionamento");
    if((errno=pthread_mutex_destroy(&(coda->lock)))) perror("Cancellazione del Mutex Lock");
    //distruggo Mutex Lock e Variabili di Condizionamento
    }
}


#endif //_queue_c_

/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file arrayLock.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#include<arrayLock.h>
#include<stdlib.h>
#include<config.h>

/**
 * @function initializeArrayLock
 * @brief inizializza un array di mutex lock
 * @param array arrayLock da inizializzare
 * @param numMutex dimensione dell'array
 * @return 0 successo, -1 errore
*/
int initializeArrayLock(arrayLock* array, int numMutex){
    array->lock=malloc(sizeof(pthread_mutex_t)*numMutex); //alloco memoria
    MEMORYCHECK(array->lock);
    for(int i=0;i<numMutex;i++)
        SYSCALLCHECK(pthread_mutex_init(&(array->lock[i]),NULL),"Inizializzazzione Macro"); //inizializzo Mutex Lock
    array->numMutex=numMutex;
    return 0;
}

/**
 * @function freeArrayLock
 * @brief libera un array di mutex lock
 * @param array arrayLock da liberare
*/
void freeArrayLock(arrayLock* array){
    if(array){
        for(int i=0;i<array->numMutex;i++)
            if((errno=pthread_mutex_destroy(&array->lock[i]))) perror("Cancellamento Mutex"); //distruggo Mutex Lock
        free(array->lock); //libero memoria

    }
}

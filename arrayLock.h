/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file arrayLock.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#include <pthread.h>

#ifndef _arraylock_h_
#define _arraylock_h_

typedef struct{
    pthread_mutex_t* lock;
    int numMutex;
} arrayLock;

/**
 * @function initializeArrayLock
 * @brief inizializza un array di mutex lock
 * @param array arrayLock da inizializzare
 * @param numMutex dimensione dell'array
 * @return 0 successo, -1 errore
*/
int initializeArrayLock(arrayLock* array, int numMutex);

/**
 * @function freeArrayLock
 * @brief libera un array di mutex lock
 * @param array arrayLock da liberare
*/
void freeArrayLock(arrayLock* array);

#define MUTEXARRAYLOCK(X,I) if((errno=pthread_mutex_lock(&(X.lock[I%(X.numMutex)])))) perror("Mutex Lock"); else{
#define MUTEXARRAYUNLOCK(X,I) if((errno=pthread_mutex_unlock(&(X.lock[I%(X.numMutex)])))) perror("Mutex Unlock");}

//L'indice è comunque calcolato in modulo alla dimensione, per evitare accessi out of bound.
//se una una pthread_mutex_lock fallisce il codice compreso tra MUTEXARRAYLOCK e MUTEXARRAYUNLOCK non viene eseguito.
//da qui le parentesi graffe alla fine


#endif

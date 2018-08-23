/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file queue.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#ifndef _queue_h_
#define _queue_h_
#include <pthread.h>

 /**
 * @struct queue
 * @brief struttura per implementare coda la coda tramite array
 * @var Table array circolare
 * @var first posizione primo elemento della coda, prossimo da essere servito
 * @var last posizione ultimo elemento della coda, ultimo da essere servito
 * @var size dimensione
 * @var nelem numero di elementi presenti nella coda
 * @var forceWakeUp 0 la coda funziona correttamente, 1 tutti i thread in wait sono stati svegliati
 * @var lock mutex lock della coda, per eseguire il codice in mutua esclusione
 * @var empty variabile di condizionamento per eseguire attesa in caso di coda vuota
 * @var full variabile di condizionamento per eseguire attesa in caso di coda piena
*/
typedef struct{
    int* Table;
    int first;
    int last;
    int size;
    int nelem;
    int forceWakeUp;
    pthread_mutex_t lock;
    pthread_cond_t empty;
    pthread_cond_t full;
} queue;

/**
 * @function initializeQueue
 * @brief funzione che inizializza la queue
 * @param coda puntatore alla coda da inizializzare;
 * @param size dimensione della coda
 * @return 0 successo, -1 errore
*/
int initializeQueue(queue* coda,int size);

/**
 * @function enqueue
 * @brief aggiunge un file descriptor alla coda
 * @param coda puntatore alla coda in cui inserire l'elemento
 * @param fd file descriptor da aggiungere alla coda
 * @returns 0 successo, -1 errore
*/
int enqueue(queue* coda,int fd);

/**
 * @function dequeue
 * @brief prende il primo elemento della coda e lo restituisce. Se la coda è vuota si mette in attesa.
 * @param coda puntatore alla coda da cui prendere l'elemento.
 * @param fd puntatore al file descriptor estratto.
 * @return 0 successo, -1 errore.
*/
int dequeue(queue* coda, int* fd);

/**
 * @function forceWakeUp
 * @brief sveglia tutti i thread in wait e fa fallire le loro enqueue e dequeue
 * @param coda puntatore alla coda da svegliare
*/
void forceWakeEveryone(queue*coda);

/**
 * @function freeQueue
 * @brief libera lo spazio occupato per la gestione della coda
 * @param coda puntatore alla coda da liberare
 * @return 0 successo, -1 errore
*/
void freeQueue(queue* coda);


#endif //_queue_h_

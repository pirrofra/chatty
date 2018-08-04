/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file history.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#ifndef _history_h_
#define _history_h_

#include <history.h>
#include <message.h>
/**
 * @struct history
 * @brief struttura dati per conservare la history dei messaggi tramite array circolare
 * @var data array di puntatori ai messaggi
 * @var pending array per segnare i messaggi ancora da consegnare
 * @var size dimensione dell'array
 * @var first posizione del primo messaggio arrivato
 * @var last posizione dell'ultimo messaggio arrivato
 * @var nel numero di elementi contenuti
*/
typedef struct{
    message_t** data;
    int* pending;
    int size;
    int first;
    int last;
    unsigned int nel;
} history;


/**
 * @function initializeHistory
 * @brief procedura per inizializzare la history
 * @param storia puntatore alla history da inizializzare
 * @param size dimensione della history
*/
void initializeHistory(history** storia, int size);

/**
 * @function addMessage
 * @brief aggiunge un messaggio alla history
 * @param storia puntatore alla history in cui inserire
 * @param mex messaggio da inserire
 * @param fd -1 se è offline, altr. è online
 * @return 0 successo, -1 errore
*/
int addMessage(history* storia, message_t* mex, int fd);



/**
 * @function freeHistory
 * @brief libera lo spazio allocato dalla History
 * @param storia puntatore alla history da liberare
*/
void freeHistory(history* storia);

#endif //_history_h_

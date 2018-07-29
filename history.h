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
 * @var size dimensione dell'array
 * @first posizione del primo messaggio arrivato
 * @last posizione dell'ultimo messaggio arrivato
*/
typedef struct{
    message_t** data;
    int size;
    int first;
    int last;
} history;


/**
 * @function initializeHistory
 * @brief funzione per inizializzare la history
 * @param storia puntatore alla history da inizializzare
 * @param size dimensione della history
 * @return 0 successo, -1 errore
*/
int initializeHistory(history** storia, int size);

/**
 * @function addMessage
 * @brief aggiunge un messaggio alla history
 * @param storia puntatore alla history in cui inserire
 * @param mex messaggio da inserire
 * @return 0 successo, -1 errore
*/
int addMessage(history* storia, message_t* mex);



/**
 * @function freeHistory
 * @brief libera lo spazio allocato dalla History
 * @param storia puntatore alla history da liberare
*/
int freeHistory(history* storia);

#endif //_history_h_

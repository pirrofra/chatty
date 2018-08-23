/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file stats.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include <stats.h>
#include <config.h>
#include<stdlib.h>

/**
 * @function initializeStats
 * @brief inizializza la struttura dati per le statistiche
 * @param stats puntatore alla struttura dati statistics da inizializzare
 * @return 0 successo, -1 errore da
*/
int initializeStats(struct statistics* stats){
    SYSCALLCHECK(pthread_mutex_init(&((stats)->lock),NULL),"Inizializzazione Mutex Lock");
    (stats)->nusers=0;
    (stats)->nonline=0;
    (stats)->ndelivered=0;
    (stats)->nnotdelivered=0;
    (stats)->nfiledelivered=0;
    (stats)->nfilenotdelivered=0;
    (stats)->nerrors=0;
    return 0;
}

/**
 * @function updusers
 * @brief aggiorna la statistica nusers
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nusers (puo' essere positivo o negativo)
*/
void updusers(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nusers += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function updonline
 * @brief aggiorna la statistica nonline
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nusers (puo' essere positivo o negativo)
*/
void updonline(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nonline += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function updelivered
 * @brief aggiorna la statistica ndelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a ndelivered (puo' essere positivo o negativo)
*/
void updelivered(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->ndelivered += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function updndelivered
 * @brief aggiorna la statistica nnotdelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nnotdelivered (puo' essere positivo o negativo)
*/
void updndelivered(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nnotdelivered += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function updfile
 * @brief aggiorna la statistica nfiledelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nfiledelivered (puo' essere positivo o negativo)
*/
void updfile(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nfiledelivered += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function updnfile
 * @brief aggiorna la statistica nfilenotdelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nfilenotdelivered (puo' essere positivo o negativo)
*/
void updnfile(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nfilenotdelivered += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function upderrors
 * @brief aggiorna la statistica nerrors
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nerrors (puo' essere positivo o negativo)
*/
void upderrors(struct statistics* stats, int n){
    LOCKACQUIRE(stats->lock);
    stats->nerrors += n;
    LOCKRELEASE(stats->lock);
}

/**
 * @function getnusers
 * @brief restituisce il numero di utenti registrati
 * @param stats  struttura che conserva le statistiche
 * @return numero di utenti registrati
*/
int getnusers(struct statistics* stats){
    int ret=0;
    LOCKACQUIRE(stats->lock);
    ret=stats->nusers;
    LOCKRELEASE(stats->lock);
    return ret;
}

/**
 * @function destroystats
 * @brief funzione che libera la struttura che conserva le statistiche
 * @param  stats  struttura che conserva le statistiche
*/
void destroystats(struct statistics* stats){
    if((errno=pthread_mutex_destroy(&stats->lock))) perror("Cancellamento Mutex"); //distrugge il mutex lock
}

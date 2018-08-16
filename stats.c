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


void updusers(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nusers += n;
    MUTEXUNLOCK(stats->lock);
}


void updonline(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nonline += n;
    MUTEXUNLOCK(stats->lock);
}



void updelivered(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->ndelivered += n;
    MUTEXUNLOCK(stats->lock);
}



void updndelivered(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nnotdelivered += n;
    MUTEXUNLOCK(stats->lock);
}



void updfile(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nfiledelivered += n;
    MUTEXUNLOCK(stats->lock);
}



void updnfile(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nfilenotdelivered += n;
    MUTEXUNLOCK(stats->lock);
}



void upderrors(struct statistics* stats, int n){
    MUTEXLOCK(stats->lock);
    stats->nerrors += n;
    MUTEXUNLOCK(stats->lock);
}

void destroystats(struct statistics* stats){
    if((errno=pthread_mutex_destroy(&stats->lock))) perror("Cancellamento Mutex");
}

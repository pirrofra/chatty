/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file stats.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>
#include<pthread.h>

struct statistics {
    unsigned long nusers;                       // n. di utenti registrati
    unsigned long nonline;                      // n. di utenti connessi
    unsigned long ndelivered;                   // n. di messaggi testuali consegnati
    unsigned long nnotdelivered;                // n. di messaggi testuali non ancora consegnati
    unsigned long nfiledelivered;               // n. di file consegnati
    unsigned long nfilenotdelivered;            // n. di file non ancora consegnati
    unsigned long nerrors;                      // n. di messaggi di errore
    pthread_mutex_t lock;
};



/* aggiungere qui altre funzioni di utilita' per le statistiche */
/**
 * @function initializeStats
 * @brief inizializza la struttura dati per le statistiche
 * @param stats puntatore alla struttura dati statistics da inizializzare
 * @return 0 successo, -1 errore da
*/
int initializeStats(struct statistics* stats);

/**
 * @function updusers
 * @brief aggiorna la statistica nusers
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nusers (puo' essere positivo o negativo)
*/
void updusers(struct statistics* stats, int n);

/**
 * @function updonline
 * @brief aggiorna la statistica nonline
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nusers (puo' essere positivo o negativo)
*/
void updonline(struct statistics* stats, int n);

/**
 * @function updelivered
 * @brief aggiorna la statistica ndelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a ndelivered (puo' essere positivo o negativo)
*/
void updelivered(struct statistics* stats, int n);

/**
 * @function updndelivered
 * @brief aggiorna la statistica nnotdelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nnotdelivered (puo' essere positivo o negativo)
*/
void updndelivered(struct statistics* stats, int n);

/**
 * @function updfile
 * @brief aggiorna la statistica nfiledelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nfiledelivered (puo' essere positivo o negativo)
*/
void updfile(struct statistics* stats, int n);

/**
 * @function updnfile
 * @brief aggiorna la statistica nfilenotdelivered
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nfilenotdelivered (puo' essere positivo o negativo)
*/
void updnfile(struct statistics* stats, int n);

/**
 * @function upderrors
 * @brief aggiorna la statistica nerrors
 * @param stats struttura che conserva le statistiche
 * @param n numero da sommare a nerrors (puo' essere positivo o negativo)
*/
void upderrors(struct statistics* stats, int n);

/**
 * @function getnusers
 * @brief restituisce il numero di utenti registrati
 * @param stats  struttura che conserva le statistiche
 * @return numero di utenti registrati
*/
int getnusers(struct statistics* stats);

/**
 * @function destroystats
 * @brief funzione che libera la struttura che conserva le statistiche
 * @param  stats  struttura che conserva le statistiche
*/
void destroystats(struct statistics* stats);

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento
 */
static inline int printStats(FILE *fout) {
    extern struct statistics chattyStats;

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		chattyStats.nusers,
		chattyStats.nonline,
		chattyStats.ndelivered,
		chattyStats.nnotdelivered,
		chattyStats.nfiledelivered,
		chattyStats.nfilenotdelivered,
		chattyStats.nerrors
		) < 0) return -1;
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */

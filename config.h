/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */
 #include <errno.h>
 #include <stdio.h>


#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH 32


/* aggiungere altre define qui */
#define NUMMUTEX 8 //Numero di Mutex per una HashTable
#define GTABLEDIM 512 //Dimensione della tabella dei gruppi
#define USERTABLEDIM 1024 //dimensione della tabella degli utenti registrati


#define MEMORYCHECK(X) if((X)==NULL){perror("Impossibile Allocare Memoria, Termino Processo"); exit(EXIT_FAILURE);}
#define SYSCALLCHECK(X,ERR) if((errno=(X))){perror(ERR);return -1;}
#define MUTEXLOCK(X) if((errno=pthread_mutex_lock(&(X)))) perror("Mutex Lock"); else{
#define MUTEXUNLOCK(X) if((errno=pthread_mutex_unlock(&(X)))) perror("Mutex Unlock");}
#define SOCKETCHECK(X) if((X)==-1){perror("Creazione Socket");return -1;}
#define TRYWRITE(X) {int errore=0; if((errore=X)==-1){perror("Write");return -1;}else if(errore==0)return 0;};
#define TRYREAD(X) {int errore=0; if((errore=X)==-1){perror("Read");return -1;}else if(errore==0)return 0;}
#define INITIALIZE(X) if((X)){printf("Errore nella Inizializzazzione, Termino il server\n");return -1;}
#define SIG_ACTION(SIG,X) if(sigaction(SIG,&X,NULL)==-1){perror("sigaction");return -1;}

// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */

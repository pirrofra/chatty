/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file fileconfig.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#ifndef _fileconfig_h_
#define _fileconfig_h_

/**
 * @struct configs
 * @var UnixPath location del socket Unix
 * @var MaxConnections numero massimo di connessioni in contemporanea
 * @var ThreadsInPool numero di thread che eseguono le operazioni
 * @var MaxMsgSize dimensione massima di un messaggio
 * @var MaxFileSize dimensione massima di un file
 * @var MaxHistMsg dimensione della hisotory dei messaggi
 * @var DirName path della directory in cui salvare i file
 * @var StatFileName nome del file delle statistiche
*/
typedef struct{
    char* UnixPath;
    int MaxConnections;
    int ThreadsInPool;
    int MaxMsgSize;
    int MaxFileSize;
    int MaxHistMsgs;
    char* DirName;
    char* StatFileName;
} configs;

/**
 * @function readConfig
 * @param path path del file di configurazione
 * @return NULL errore, puntatore a una struttra configs in caso di successo
*/
configs* readConfig(char* path);

configs* initializeConfig();

int checkC(configs*);

void freeC(configs*);


#endif //_fileconfig_h_
